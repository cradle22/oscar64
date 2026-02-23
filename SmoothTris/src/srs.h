#ifndef SRS_H
#define SRS_H

int check_collision(char piece_idx, int rot, char pos_x, char pos_y);
bool try_rotate(int *current_rot, char piece_idx, char *pos_x, char *pos_y, char dir);


#pragma compile("srs.c")

#endif