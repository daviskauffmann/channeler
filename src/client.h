#ifndef CLIENT_H
#define CLIENT_H

int client_main(int argc, char *argv[]);
void client_accelerate(float *pos_x, float *pos_y, float *vel_x, float *vel_y, float *acc_x, float *acc_y, float delta_time);

#endif
