#ifndef NFLSTREET_THREE_PAIR_PROJECTION_H
#define NFLSTREET_THREE_PAIR_PROJECTION_H

/* Six input pairs and three scalar coefficients produce three outputs. */
void nflstreet_three_pair_projection(const float source[12],
                                     const float coefficients[3],
                                     float output[3]);

#endif
