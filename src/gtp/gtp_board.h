#ifndef GTP_BOARD_H
#define GTP_BOARD_H

#include <string>
#include "../goboard.h"

std::string coords_to_gtp_position(Move);
Move gtp_position_to_move(const std::string&);


#endif // GTP_BOARD_H
