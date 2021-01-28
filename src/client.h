#ifndef CLIENT_H
#define CLIENT_H

int client_main(int argc, char *argv[]);
void client_move(float dx, float dy, float delta_time, float *new_x, float *new_y);

#endif
