#ifndef DHT22_H_
#define DHT22_H_

int init_dht();

int get_dht_data(float *temp_cels, float *temp_fahr, float *humidity);

#endif /* DHT22_H_ */
