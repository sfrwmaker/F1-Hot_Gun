/*
 * msg.cpp
 *
 *  Created on: Oct 12, 2025
 *      Author: Alex
 */

#include "msg.h"

const char*	MSG::msg(t_msg_id id) {
	if (id >= MSG_LAST)
		return 0;
	return message[id];
}

uint8_t MSG::menuSize(t_msg_id id) {
	uint8_t ret = 0;
	switch (id) {
		case MSG_MENU_MAIN:
			ret = MSG_NONE - MSG_MENU_MAIN -1;
			break;
		default:
			break;
	}
	return ret;
}
