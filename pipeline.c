// File: hw5.c 
// Solution by Chris Turgeon 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUF 128 // ASSUMPTION: LINE BUFFER SIZE
#define LEN 5   // ASSUMPTION: # OF LINES

typedef enum { false, true } bool;



struct stall {
	char stall_reg[4];    // Duplicate register.
	int instruc_type;     // int code for instruction.

	int line_occur;       // Line number where data dependency lies.
	int line_cause;       // Line number where data comes from.
	int out[10];          // Array to output nops row. 9th index turns on output.

	int num_nops;         // Number of nops that should be  
};                       // printed per dependency. 



// This function advances an instruction to its next stage.
void update_cycles(int data[LEN][14], int stage_cnt, int curr) {
	// Update current instruction for next stage.
	if ( stage_cnt < curr || data[curr][12] == 1 ) {
		return;
	} else {
		int idx = data[curr][11];
		if ( curr != 0 ) { // If not instruction one.
			if ( data[curr][idx - 1] != 5 ) {
				data[curr][idx] = data[curr][idx - 1] + 1;
				data[curr][11]++; 
			}
		} else { // If instruction one. 
			data[curr][idx + 1] = data[curr][idx] + 1;
			data[curr][11]++; 
		}
	}
}



// Counts the number of a desired register in the input code. Takes in the 
// starting line and searches down for it.
int count_reg(char input[LEN][BUF], int start_instruc, int instruc_cnt, char reg[4]) {
	int cnt = 0;
	int i, j;
	for (i = start_instruc; i < instruc_cnt; i++) {
		// Advance past left most register.
		int tmp = 0;
		while ( input[i][tmp] != '$' ) {
			tmp++;
		}
		tmp++;
		for (j = tmp; input[i][j] != '\0'; j++) {
			bool char_match_one = false;
			bool char_match_two = false;
			if ( input[i][j] == '$' ) {
				char_match_one = input[i][j+1] == reg[1];
				char_match_two = input[i][j+2] == reg[2];
			}
			if ( char_match_one && char_match_two ) {
				cnt++;
			}
		}
	}
	return cnt;
}



// This function stalls an instruction until it has the available data.
// It gets called while waiting for WB from dependency instruction.
void update_stall_cycles(int data[LEN][14], int instruc_num, 
								 int instruc_cnt, int cause_line, int stage_cnt) {


	// Find location of WB in the dependency cause line
	int loc = 0;
	while (data[cause_line][loc] != 1) {
		loc++;
	}
	loc += 4;

	// Update stall causing instruction to get to WB.
	if ( data[cause_line][loc-1] == 4 ) {
		data[cause_line][loc] = 5;
	}

	int i, j;
	for (i = instruc_num; i < instruc_cnt; i++) {
		j = 0;
		while (data[i][j] == 0) { // Advance to IF
			j++;
		}
		while (data[i][j] != 0) { // Advance to end of stages.
			j++;
		}

		// For the first nop print, decrement value and turn 
		// on the output int and the first done int.
		if ( data[i][13] == 0 ) {
			data[i][j-1]--;
			data[i][13] = 1;
			data[i][12] = 1;
		} else { 
			if ( j - 1 < loc ) { // If loc not at WB, repeat stage.
				if ( data[i][j-1] == data[i][j-2] ) {
					data[i][j] = data[i][j-1];
					data[i][11]++;
				}
			} 
			else if ( j - 1 == loc ) { // Advance stage.
				data[i][j] = data[i][j-1] + 1;
				data[i][11]++;
			} else { // Call function to advance stage.
				data[i][12] = 0;
				update_cycles(data, stage_cnt, i);
			}
		}
	}
}



// Returns the index within an instruction output when it finds desired stage.
int find_index(int data[LEN][14], int instruc_code, int instruc_num) {
	int i;
	for (i = 0; i < 9; i++) {
		if ( data[instruc_num][i] == instruc_code ) {
			return i;
		}
	}
	return -1;
}



// This function searches an instruction to see if it has activated a stage.
bool search_instruc(int data[LEN][14], int instruc_code, int instruc_num) {
	int i;
	for (i = 0; i < 9; i++) {
		if ( data[instruc_num][i] == instruc_code ) {
			return true;
		}
	}
	return false;
}



// This function updates the nops by adding IF ID and *'s'
void update_nop(int nop_data[9], int starting_idx) {
	int i = starting_idx;
	int col_cnt = 0;
	while ( nop_data[i] != 0 ) {
		// Count the number of stages in the nop.
		if (nop_data[i] != 0) {
			col_cnt++;
		} 
		// Return if 5 stages of nop done.
		if ( col_cnt == 5 ) {
			return;
		}
		i++;
	}
	// Add a '*' to nop output
	nop_data[i] = 6;
}


// This function initializes the nop at the starting index provided.
void initialize_nop(int nop_data[9], int starting_idx) {
	if ( nop_data[starting_idx] != 0 ) {
		return;
	}
	nop_data[starting_idx] = 1;   // IF
	nop_data[starting_idx+1] = 2; // ID
	nop_data[starting_idx+2] = 6; // *
}



int main( int argc, char *argv[] ) {

	// Check to make sure that there are proper number of arguments.
	if ( argc != 2 ) {
		perror("Error: Invalid Number of Arguments!");
		return EXIT_FAILURE;
	}

	// Open file and check to see if it's opened correctly.
	FILE *file;
	if ( (file = fopen(argv[1], "r")) == NULL ) {
		perror("Error: Inavlid Input File!\n");
		return EXIT_FAILURE;
	} 

	// Array to hold stages.
	char *processes[7] = { ".", "IF", "ID", "EX", "MEM", "WB", "*" };

	// Ouput array to be updated.
	int output[LEN][14];
	int x, y;
	for (x = 0; x < LEN; x++) {
		for (y = 0; y < 12; y++) {
			output[x][y] = 0;
		}
		output[x][11] = x;
	}
	output[0][0] = 1;

	/* For the first 9 digits in the 14 digit array 
		 * 0 - indicates period - "."
	    * 1 - indicates IF
	    * 2 - indicates ID
	    * 3 - indicates EX
	    * 4 - indicated MEM
	    * 5 - indicates WB
	    * 6 - indicates *
	 */

	/* For the 10th integer in the 14 digit array
		 * 0 - add
		 * 1 - sub
		 * 2 - and
		 * 3 - or
		 * 4 - lw
		 * 5 - sw
    */

	/* For the 11th digit in the 14 digit array
		 *  -1 - indicates row is complete
	   For the 12th digit in the 14 digit array
	    *  - indicates the starting index and IF location 
	   For the 13th digit in the 14 digit array
	    *  1 - indicates that the update_stalls func should not update
	    For 14th digit, 1 indicates first op has been completed
	 */

	// Needed for input.
	int i = 0;
	char input_code[LEN][BUF]; 
	char buffer[BUF];              

	while ( fgets(buffer, BUF, file) ) {

		// Assign proper integer to represent instruction.
		if ( buffer[0] == 'a' && buffer[1] == 'd' && buffer[2] == 'd' ) 
			output[i][9] = 0;
		else if ( buffer[0] == 's' && buffer[1] == 'u' && buffer[2] == 'b' ) 
			output[i][9] = 1;
		else if ( buffer[0] == 'a' && buffer[1] == 'n' && buffer[2] == 'd' ) 
			output[i][9] = 2; 
		else if ( buffer[0] == 'o' && buffer[1] == 'r' )
			output[i][9] = 3; 
		else if ( buffer[0] == 'l' && buffer[1] == 'w' ) 
			output[i][9] = 4;
		else 
			output[i][9] = 5;
		
		// Read in the file line-by-line; replace \n with \0.
		output[i][12] = 0;
		output[i][13] = 0;
		buffer[strlen(buffer) - 1] = '\0';
		strcpy(input_code[i], buffer);
		i++;
	}
	int instruc_cnt = i;


	///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// 

	      /* SEARCH FOR STALLS AND STORE THEIR INFORMATION */

	///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// 


	int stall_cnt = 0;
	struct stall stalls[LEN];	

	// Loop through the input code and search for register duplicates.
	for (i = 0; i < instruc_cnt; i++) {
		// Find the first register.
		int reg_one_idx = 0;
		while ( input_code[i][reg_one_idx] != '$' ) {
			reg_one_idx++;
		}
		// Store the register as a string.
		char reg[4];
		reg[0] = input_code[i][reg_one_idx];
		reg[1] = input_code[i][reg_one_idx + 1]; 
		reg[2] = input_code[i][reg_one_idx + 2];
		reg[3] = '\0';

		// Search through lower instrutions for the same, stall-causing register.
		int instruc;
		for (instruc = i + 1; instruc < instruc_cnt; instruc++) {

			// Move to the next instruction if dependency line is too far below.
			if ( instruc - i >= 3 ) {    
				continue;
			}  

			// If instruction is not lw or sw, then skip past left-most register.
			int tmp = 0;
			if ( (output[instruc][9] != 4) || (output[instruc][9] !=  5) ) {
				while ( input_code[instruc][tmp] != '$' ) {
					tmp++;
				}
				tmp++;
			}

			// Compare the stored register with the found one.
			int j;
			for (j = tmp; input_code[instruc][j] != '\0'; j++) {
				bool char_match_one = false;
				bool char_match_two = false;
				if ( input_code[instruc][j] == '$' ) {
					char_match_one = input_code[instruc][j+1] == reg[1];
					char_match_two = input_code[instruc][j+2] == reg[2];
				}
				// Construct stall struct object and add to array.
				if ( char_match_one && char_match_two ) {

					// Search to see if there are multiple dependencies on one register.
					bool found_duplicate = false;
					int tmp, dup_index;
					for (tmp = 0; tmp < stall_cnt; tmp++) {
						if ( stalls[tmp].line_cause == i ) {
							dup_index = tmp;
							found_duplicate = true;
						}
					}

					// Create a new unique stall and assign values.
					if ( !found_duplicate ) {
						strcpy( stalls[stall_cnt].stall_reg, reg); 
						stalls[stall_cnt].instruc_type = output[i][9]; 
						stalls[stall_cnt].line_occur = instruc;
						stalls[stall_cnt].line_cause = i;

						// Dependency occurs in the next line and there are less than 2.
						int reg_cnt = count_reg(input_code, instruc, instruc_cnt, reg);
						if ( (instruc - i == 1) && reg_cnt < 2 ) { 
							stalls[stall_cnt].num_nops = 2;
						} else { // Dependency occurs in later line.
							stalls[stall_cnt].num_nops = 1;
						}
						
						// Initialize all values to 0.
						int k;
						for (k = 0; k < 9; k++) {
							stalls[stall_cnt].out[k] = 0;
						}
						stalls[stall_cnt].out[9] = 0;
						stall_cnt++;
					}

					// Increment stall count for already found dependency.
					else {
						stalls[dup_index].num_nops += 1;
					}
				}
			}
		}
	}


   ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// ///// 

	        /* BEGIN OUTPUT OF THE MIPS PIPELINE  */  

	///// ///// ///// ///// ///// ///// ///// ///// ///// ///// /////  
	
	
	int stage_cnt = 1;	// Keep track of current stage.
	printf("START OF SIMULATION\n\n");

	bool done_pipelining = false;
	while ( !done_pipelining ) {

		// Print cycle numbers.
		printf("CPU Cycles ===>\t");
		int cycle;
		for (cycle = 1; cycle != 10; cycle++) {
			if ( cycle < 9 ) { printf("%d\t", cycle); }
			else { printf("%d\n", cycle); }
		}

		bool finished_curr_stage = false;
		while ( !finished_curr_stage ) {
			// Print current instruction MIPS code.
			for (i = 0; i < instruc_cnt; i++) {

				printf("%s\t\t", input_code[i]);
				// Loop through current instruction and print stages.
				int j;
				for (j = 0; j < 9; j++) {
					// Update -1 status to indicate row is complete, found WB.
					if ( output[i][j] == 5 ) {
						output[i][10] = -1;
					}
					if ( j < 8 ) {
						printf("%s", processes[ output[i][j] ]);
						printf("\t");
					} else {
						printf("%s", processes[ output[i][j] ]);
						printf("\n");
					}
				}

			/////	///// ///// ///// ///// ///// ///// /////

	         /*	DETERMINE IF STALLS ARE NEEDED HERE */	

	      ///// ///// ///// ///// ///// ///// ///// /////

				for (j = 0; j < stall_cnt; j++) {

					/* "at_ID" checks to see if the stall-waiting line has reached ID stage.
					 * "WB_done" checks to see if the stall-causing line has written back.
					 * "at_stall" location determined by top-most dependency.
					 */
					bool at_ID = search_instruc(output, 2, stalls[j].line_occur); 
					bool WB_done = output[ stalls[j].line_cause ][10] == -1;  
					bool at_stall = (i == stalls[j].line_occur - 1); 

					/* Find the starting index to initialize stall object for output.
					 * Initialize the IF ID * if needed, then print, then update the 
					 * nop instruction to fill in additional *'s if needed.
					 */
					if ( stalls[j].out[9] == 1 && at_stall ) {
						int idx_start = find_index(output, 1, stalls[j].line_occur);
						initialize_nop(stalls[j].out, idx_start); 

						// Print out the proper number of nops.
						int num_nops;
						for (num_nops = 0; num_nops < stalls[j].num_nops; num_nops++) {
							printf("nop\t\t");
							int k;
							for (k = 0; k < 8; k++) {
								printf("%s\t", processes[ stalls[j].out[k] ] );
							}
							printf("%s\n", processes[ stalls[j].out[k] ]);
						}
						update_nop(stalls[j].out, idx_start);
						update_stall_cycles(output, i + 1, instruc_cnt, stalls[j].line_cause, stage_cnt);
					} 
					// Turn on output for stall.
					if ( at_stall && !WB_done && at_ID ) {
						stalls[j].out[9] = 1;
					} 
				} 

				// Update the output to be ready for next stage. 
				if ( output[i][10] != -1 ) {
					update_cycles(output, stage_cnt, i); 
				} 
			}
			finished_curr_stage = true;
		}
		printf("\n");
		stage_cnt++;

		// Pipeline complete when last instruction completely printed.
		if ( output[instruc_cnt - 1][10] == -1 ) {
			printf("END OF SIMULATION\n");
			done_pipelining = true;
		}
	}
	return EXIT_SUCCESS;
}