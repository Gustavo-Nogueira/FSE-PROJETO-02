#include <ncurses.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cJSON.h"
#include "definitions.h"
#include "sockets.h"
#include "sysstate.h"

#define KEY_SELECT_OPT '\n'

#define MIN_COLS 110
#define MIN_LINES 40

#define STATE_WD_WIDTH 1
#define STATE_WD_HEIGHT 0.25
#define MENU_WD_WIDTH 1  // 0.5
#define MENU_WD_HEIGHT 0.75
#define LOGS_WD_WIDTH 0.5
#define LOGS_WD_HEIGHT 0.75

#define SIZEOF_OPTION_LIST(L) (sizeof(L) / sizeof(MENU_OPTION))

typedef struct {
    void *arg;
    void (*handler)(void *arg);
} OPTION_PRESS_HANDLER;

typedef struct {
    int server_id;
    int gpio;
} TOGGLE_DEVICE_ARG;

typedef struct {
    int key;
    char label[500];
    OPTION_PRESS_HANDLER press_handler;
} MENU_OPTION;

typedef struct {
    int server_id;
    MENU_OPTION options[100];
} SERVER_COMMAND_PANEL;

sem_t screen_mutex;
int app_lines, app_cols;
WINDOW *win_state, *win_menu, *win_log;
MENU_OPTION main_menu_opts[100];
SERVER_COMMAND_PANEL command_panels[N_DISTR_SERVERS];

static void refresh_window(WINDOW *_wdw) {
    sem_wait(&screen_mutex);
    wrefresh(_wdw);
    sem_post(&screen_mutex);
}

static void delete_window(WINDOW *_wdw) {
    sem_wait(&screen_mutex);
    werase(_wdw);
    wrefresh(_wdw);
    delwin(_wdw);
    sem_post(&screen_mutex);
}

static void update_dimensions_data() {
    app_lines = LINES > MIN_LINES ? LINES : MIN_LINES;
    app_cols = COLS > MIN_COLS ? COLS : MIN_COLS;
}

static void draw_selection_box(int _lns, int _cls, int _by, int _bx, char *title, MENU_OPTION lst[], size_t sz_lst, int init_k) {
    int pos, ch;
    char item[1000];
    char format[10];
    const int margin_horz = 2;
    const int margin_vert = 3;
    sprintf(format, "%%-%ds", _cls - 2 * margin_horz);

    WINDOW *subw = newwin(_lns, _cls, _by, _bx);
    // box(subw, 0, 0);

    wattron(subw, A_BOLD);
    mvwprintw(subw, 1, margin_horz, "%s", title);
    wattroff(subw, A_BOLD);

    for (int i = 0; i < sz_lst; i++) {
        sprintf(item, format, lst[i].label);
        if (lst[i].key == init_k) {
            pos = i;
            wattron(subw, A_STANDOUT);
            mvwprintw(subw, i + margin_vert, margin_horz, "%s", item);
            wattroff(subw, A_STANDOUT);
        } else {
            mvwprintw(subw, i + margin_vert, margin_horz, "%s", item);
        }
    }

    refresh_window(subw);

    noecho();
    keypad(subw, TRUE);
    curs_set(0);

    while ((ch = wgetch(subw)) != KEY_SELECT_OPT) {
        // right pad with spaces to make the items appear with even width.
        sprintf(item, format, lst[pos].label);
        mvwprintw(subw, pos + margin_vert, margin_horz, "%s", item);

        switch (ch) {
            case KEY_UP:
                pos--;
                pos = (pos < 0) ? sz_lst - 1 : pos;
                break;
            case KEY_DOWN:
                pos++;
                pos = (pos >= sz_lst) ? 0 : pos;
                break;
            default:
                break;
        }

        // now highlight the next item in the list.
        wattron(subw, A_STANDOUT);
        sprintf(item, format, lst[pos].label);
        mvwprintw(subw, pos + margin_vert, margin_horz, "%s", item);
        wattroff(subw, A_STANDOUT);
    }

    delete_window(subw);

    lst[pos].press_handler.handler(lst[pos].press_handler.arg);
}

static void handle_press_toggle_alarm(void *arg) {
    SYSTEM_STATE *sysstate = get_system_state();
    set_alarm((sysstate->alarm ? 0 : 1));
}

static void handle_press_change_view(void *arg) {
    int server_id = *((int *)arg);
    set_current_view(server_id);
}

static void handle_press_command_panel(void *arg) {
    int server_id = *((int *)arg);
    int x_offset = 0;
    int y_offset = app_lines * STATE_WD_HEIGHT;
    int box_cols = app_cols * MENU_WD_WIDTH;
    int box_lines = app_lines * MENU_WD_HEIGHT;

    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        if (command_panels[i].server_id == server_id) {
            draw_selection_box(box_lines - 2, box_cols - 2, y_offset + 1, x_offset + 1,
                               "Painel de comando", command_panels[i].options, SIZEOF_OPTION_LIST(command_panels[i].options), 1);
            break;
        }
    }
}

static void handle_press_toggle_device(void *arg) {
    char *json_request;
    struct sockaddr_in addr;
    cJSON *cj_request = NULL, *cj_data = NULL, *cj_item = NULL;
    SYSTEM_STATE *sysstate = get_system_state();
    int server_id = ((TOGGLE_DEVICE_ARG *)arg)->server_id;
    int gpio = ((TOGGLE_DEVICE_ARG *)arg)->gpio;
    int value = 0;

    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        if (sysstate->distr_servers[i].server_id == server_id) {
            create_sockaddr(sysstate->distr_servers[i].address.ip, sysstate->distr_servers[i].address.port, &addr);
            for (int j = 0; j < sysstate->distr_servers[i].size_outputs; j++) {
                if (sysstate->distr_servers[i].outputs[j].gpio == gpio) {
                    value = sysstate->distr_servers[i].outputs[j].current_value ? 0 : 1;
                }
            }
            break;
        }
    }

    cj_data = cJSON_CreateObject();
    cj_request = cJSON_CreateObject();

    cj_item = cJSON_CreateNumber(CMD_SET_DEVICE_VALUE);
    cJSON_AddItemToObject(cj_request, "command", cj_item);

    cj_item = cJSON_CreateNumber(gpio);
    cJSON_AddItemToObject(cj_data, "gpio", cj_item);

    cj_item = cJSON_CreateNumber(value);
    cJSON_AddItemToObject(cj_data, "value", cj_item);

    cJSON_AddItemToObject(cj_request, "data", cj_data);

    json_request = cJSON_Print(cj_request);

    send_request(json_request, &addr, NULL);

    free(json_request);
    cJSON_Delete(cj_request);
}

static void handle_press_exit(void *arg) {
    kill(getpid(), SIGINT);
}

static void build_menu_structure() {
    int *server_id;
    int indexm = 0;
    TOGGLE_DEVICE_ARG *tdevice_arg;
    SYSTEM_STATE *sysstate = get_system_state();

    // TOGGLE ALRM OPTION
    main_menu_opts[indexm].key = indexm;
    sprintf(main_menu_opts[indexm].label, "Toggle Alarme");
    main_menu_opts[indexm].press_handler.arg = NULL;
    main_menu_opts[indexm].press_handler.handler = &handle_press_toggle_alarm;

    // CHANGE VIEW STATE OPTIONS
    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        main_menu_opts[indexm].key = indexm;
        sprintf(main_menu_opts[indexm].label, "Visualizar estado %s", sysstate->distr_servers[i].name);
        server_id = (int *)malloc(sizeof(int));
        *server_id = sysstate->distr_servers[i].server_id;
        main_menu_opts[indexm].press_handler.arg = server_id;
        main_menu_opts[indexm].press_handler.handler = &handle_press_change_view;
        indexm++;
    }

    // OPEN COMMAND PANEL OPTIONS
    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        main_menu_opts[indexm].key = indexm;
        sprintf(main_menu_opts[indexm].label, "Painel de comando %s", sysstate->distr_servers[i].name);

        server_id = (int *)malloc(sizeof(int));
        *server_id = sysstate->distr_servers[i].server_id;
        main_menu_opts[indexm].press_handler.arg = server_id;
        main_menu_opts[indexm].press_handler.handler = &handle_press_command_panel;
        command_panels[i].server_id = sysstate->distr_servers[i].server_id;

        for (int j = 0; j < sysstate->distr_servers[i].size_outputs; j++) {
            command_panels[i].options[j].key = j;
            sprintf(command_panels[i].options[j].label, "Toggle %s", sysstate->distr_servers[i].outputs[j].tag);
            tdevice_arg = (TOGGLE_DEVICE_ARG *)malloc(sizeof(TOGGLE_DEVICE_ARG));
            tdevice_arg->gpio = sysstate->distr_servers[i].outputs[j].gpio;
            tdevice_arg->server_id = sysstate->distr_servers[i].server_id;
            command_panels[i].options[j].press_handler.arg = tdevice_arg;
            command_panels[i].options[j].press_handler.handler = &handle_press_toggle_device;
        }
        indexm++;
    }

    // EXIT PROGRAM OPTION
    main_menu_opts[indexm].key = indexm;
    sprintf(main_menu_opts[indexm].label, "Encerrar Programa (CTRL+C)");
    main_menu_opts[indexm].press_handler.arg = NULL;
    main_menu_opts[indexm].press_handler.handler = &handle_press_exit;
}

void draw_menu(WINDOW *_wdw) {
    int x_offset = 0;
    int y_offset = app_lines * STATE_WD_HEIGHT;
    int box_cols = app_cols * MENU_WD_WIDTH;
    int box_lines = app_lines * MENU_WD_HEIGHT;

    delete_window(_wdw);

    _wdw = newwin(box_lines, box_cols, y_offset, x_offset);
    box(_wdw, 0, 0);

    wattron(_wdw, A_BOLD);
    mvwprintw(_wdw, 0, 1, "Menu");
    wattroff(_wdw, A_BOLD);
    refresh_window(_wdw);

    while (1) {
        draw_selection_box(box_lines - 2, box_cols - 2, y_offset + 1, x_offset + 1,
                           "Menu Principal", main_menu_opts, SIZEOF_OPTION_LIST(main_menu_opts), 1);
    }
}

void draw_system_state(WINDOW *_wdw) {
    char tmp[500];
    int px, py;
    int box_cols = app_cols * STATE_WD_WIDTH;
    int box_lines = app_lines * STATE_WD_HEIGHT;
    int start_y = (box_lines - 7) / 2;
    SYSTEM_STATE *sysstate = get_system_state();

    delete_window(_wdw);

    curs_set(0);

    _wdw = newwin(box_lines, box_cols, 0, 0);
    box(_wdw, 0, 0);

    wattron(_wdw, A_BOLD);
    mvwprintw(_wdw, 0, 1, "Estado");
    wattroff(_wdw, A_BOLD);

    // LINE 01
    sprintf(tmp, "ALARME: %s", sysstate->alarm ? "On" : "Off");
    px = start_y;
    py = ((box_cols / 4) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    sprintf(tmp, "TEMPERATURA: %.2lf", sysstate->temperature);
    px = start_y;
    py = (box_cols / 4) * 1 + ((box_cols / 4) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    sprintf(tmp, "HUMIDADE: %.2lf", sysstate->humidity);
    px = start_y;
    py = (box_cols / 4) * 2 + ((box_cols / 4) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    sprintf(tmp, "OCUPACAO: %d", sysstate->occupation);
    px = start_y;
    py = (box_cols / 4) * 3 + ((box_cols / 4) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    // INPUTS AND OUTPUTS
    int sindex = 0;
    for (int i = 0; i < N_DISTR_SERVERS; i++) {
        if (sysstate->current_view == sysstate->distr_servers[i].server_id) {
            sindex = i;
            break;
        }
    }

    sprintf(tmp, "ENTRADAS: %s", sysstate->distr_servers[sindex].name);
    px = start_y + 1;
    py = ((box_cols / 2) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    sprintf(tmp, "SAIDAS: %s", sysstate->distr_servers[sindex].name);
    px = start_y + 1;
    py = (box_cols / 2) + ((box_cols / 2) / 2) - (strlen(tmp) / 2);
    mvwprintw(_wdw, px, py, tmp);

    int mxsize = MAX(sysstate->distr_servers[sindex].size_inputs, sysstate->distr_servers[sindex].size_outputs);
    for (int i = 0; i < mxsize; i++) {
        if (i < sysstate->distr_servers[sindex].size_inputs) {
            sprintf(tmp, "%s: %s", sysstate->distr_servers[sindex].inputs[i].tag,
                    sysstate->distr_servers[sindex].inputs[i].current_value ? "On" : "Off");
            px = start_y + i + 2;
            py = ((box_cols / 2) / 2) - (strlen(tmp) / 2);
            mvwprintw(_wdw, px, py, tmp);
        }
        if (i < sysstate->distr_servers[sindex].size_outputs) {
            sprintf(tmp, "%s: %s", sysstate->distr_servers[sindex].outputs[i].tag,
                    sysstate->distr_servers[sindex].outputs[i].current_value ? "On" : "Off");
            px = start_y + i + 2;
            py = (box_cols / 2) + ((box_cols / 2) / 2) - (strlen(tmp) / 2);
            mvwprintw(_wdw, px, py, tmp);
        }
    }

    refresh_window(_wdw);
}

void call_draw_menu() {
    draw_menu(win_menu);
}

void call_draw_system_state() {
    draw_system_state(win_state);
}

void init_screen() {
    initscr();
    refresh();

    sem_init(&screen_mutex, 0, 1);

    build_menu_structure();

    update_dimensions_data();
}

void close_screen() {
    if (win_state != NULL) delwin(win_state);
    if (win_menu != NULL) delwin(win_menu);
    if (win_log != NULL) delwin(win_log);

    endwin();
}