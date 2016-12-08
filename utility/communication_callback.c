/* bricklib2
 * Copyright (C) 2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication_callback.c: Helper functions for Bricklet communication
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "communication_callback.h"

#include "bricklib2/hal/system_timer/system_timer.h"

static CommunicationCallbackListNode communication_callback_list_nodes[COMMUNICATION_CALLBACK_HANDLER_NUM] = {
	COMMUNICATION_CALLBACK_LIST_INIT
};

static CommunicationCallbackListNode *communication_callback_list_start = &communication_callback_list_nodes[0];

void communication_callback_tick(void) {
	static uint32_t last_tick = 0;

	if(!system_timer_is_time_elapsed_ms(last_tick, COMMUNICATION_CALLBACK_TICK_WAIT_MS)) {
		return;
	}
	last_tick = system_timer_get_ms();

	// Start with start node
	CommunicationCallbackListNode *current = communication_callback_list_start;

	// Here we save the first node that was moved to the end.
	// We stop as soon as we see this node. If we don't move any nodes
	// we stop as soon as we see NULL, which is correct
	CommunicationCallbackListNode *end_of_orig_list = NULL;

	do {
		// Save next of current node
		CommunicationCallbackListNode *next = current->next;

		if(current->handler()) {
			// If the callback handler did send a message, we try to
			// move the handler to the end of the list

			// If we are the start node, the next node will be the start node
			// from now on
			if(communication_callback_list_start == current) {
				communication_callback_list_start = next;
				next->prev = NULL;
			}

			// Find the end of the whole list
			CommunicationCallbackListNode *end = current;
			while(end->next != NULL) {
				end = end->next;
			}

			// If we already are at the end, there is nothing to do
			if(end != current) {
				// If we didn't move anything to the end before, the current node
				// will be the end of the outer loop
				if(end_of_orig_list == NULL) {
					end_of_orig_list = current;
				}

				// Otherwise we put the current node at the end
				end->next = current;

				// If we had a prev node, we have to update the next node of it
				if(current->prev != NULL) {
					current->prev->next = current->next;
				}

				// Update the next and prev node of the current node (now end node)
				current->next = NULL;
				current->prev = end;
			}
		}

		current = next;

	} while(current != end_of_orig_list);
}

void communication_callback_init(void) {
	for(uint32_t i = 0; i < COMMUNICATION_CALLBACK_HANDLER_NUM-1; i++) {
		communication_callback_list_nodes[i].next = &communication_callback_list_nodes[i+1];
		communication_callback_list_nodes[i+1].prev = &communication_callback_list_nodes[i];
	}

	communication_callback_list_nodes[0].prev = NULL;
	communication_callback_list_nodes[COMMUNICATION_CALLBACK_HANDLER_NUM-1].next = NULL;
}
