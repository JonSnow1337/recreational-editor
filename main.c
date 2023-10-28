#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#define _GNU_SOURCE

int x = 0;
int y = 0;
char **strings ={0};
bool is_running = true;
int maxY = 1;
char *input_file_name;
int loaded_file_lines = 0;
WINDOW *menu_window;
bool menu_on;

bool isOkChar(char c){
    return (c >= 32 || c <127) && c != KEY_BACKSPACE;
}

void disable_flow_control() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_iflag &= ~(IXON | IXOFF);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void loadFile(char *fileName){
    FILE *fileptr;
    ssize_t read;
    size_t len = 0;
    char *line;

    fileptr = fopen(fileName, "r");
    while ((read = getline(&line, &len, fileptr)) != -1) {
       	strcpy(strings[loaded_file_lines], line);

        loaded_file_lines ++;
    }
}
WINDOW *create_newwin(int height, int width, int starty, int startx)
{	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	box(local_win, 0 , 0);		/* 0, 0 gives default characters 
					 * for the vertical and horizontal
					 * lines			*/
    wborder(local_win, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(local_win);		/* Show that box 		*/

    return local_win;
}

void save_file(char * file_name){
    FILE *fptr;
    fptr = fopen(file_name, "w");
    for (int i = 0; i < maxY + 1; i++){
        fprintf(fptr,"%s", strings[i]);
    }
    fclose(fptr);
}
void deleteMenu(){
    menu_on = !menu_on;
    delwin(menu_window);
    menu_window = NULL;
    touchwin(stdscr);
    refresh();


}
void do_menu(){
    int oldX = x;
    int oldY = y;
    if(menu_window == NULL){
        noecho();
        menu_window = create_newwin(20,20,y,x);
        int optsLen = 2;
        mvwprintw(menu_window,1,0, "| 1.Save");
        mvwprintw(menu_window,2,0, "| 2.Send Packet");

        y += optsLen + 1;
        x ++;
        move(y,x);
        wrefresh(menu_window);
        menu_on = true;
        char ch;
        while (menu_on){
            ch = getch();
            if(ch == '1'){
                mvwprintw(menu_window,optsLen + 1,0, "Enter Filename: ");
                y += 1;
                move(y,x);
                wrefresh(menu_window);
                char file_name [256] = {0};
                int cntr = 0;
                int c;
                while ((c = getch()) != '\n'){
                    if (isOkChar(c)){
                        file_name[cntr] = (char)c;
                        mvwaddch(menu_window,y,x,c);
                        x++;
                        wrefresh(menu_window);
                        cntr ++;

                    } 
                }
                save_file(file_name);
                deleteMenu();

            }
            else if(ch == '2') {

            }
            else{
                menu_on = false;
            }
        }
    }
    if( !menu_on){
        deleteMenu();
        move(oldY, oldX);
        menu_on = false;
    }

}

int main(int argc, char *argv[]){

    disable_flow_control();
    int numOfLines = 50;
    int charsPerLine = 200;
    strings = (char**)malloc(numOfLines * sizeof(char*));
    for (int i = 0; i < numOfLines; i++){
        strings[i] = (char*)malloc(charsPerLine * sizeof(char));
        memset(strings[i], 0, charsPerLine);
    }

    initscr();
    curs_set(true);
    keypad(stdscr,true);
    noecho();

    if(argc > 1) {
        input_file_name = argv[1];
        printf("%s", input_file_name);
        loadFile(input_file_name);
        for (int i = 0 ; i < loaded_file_lines; i++)
        {
          printw(strings[i]);

        }
    }
    while (is_running) {

        wrefresh(stdscr);

        int ch = getch();
        if (ch == KEY_LEFT){
            if(strings[y][x] == 0){
                strings [y][x] = ' ';
                addch(' ');
            }

            x --;
            move(y,x);
        }
        else if(ch == KEY_RIGHT || ch == 32 ){
            if(strings[y][x] == 0){
                strings [y][x] = ' ';
                addch(' ');
            }

            x ++;
            move(y,x);
        }
        else if(ch == KEY_DOWN){
            strings[y][x] = '\n';
            for(int i =0; i < x-1; i++){
                if(strings[y][i] == 0){
                    strings[y][i] = ' ';
                }
            }
            y ++;
            move(y,x);      
        }
        else if(ch == KEY_UP){
            y --;
            move(y,x);
        }
        else if(ch == '\n') {
            strings[y][x] = '\n';
            strings[y][x+1] = '\0';
            x = 0;
            y ++;
            move(y,x);

        }
        else if(ch == KEY_BACKSPACE) {
            delch();
            x --;
            move(y,x);
        }
        else if(ch == 19){
            //ctrl+s
            endwin();
            FILE *fptr;
            char filename [256];
            printf("Enter file name: ");
            scanf("%s",filename);
            fptr = fopen(filename, "w");
            for (int i = 0; i < maxY + 1; i++){
                fprintf(fptr,"%s", strings[i]);
            }
            fclose(fptr);
            is_running = false;
        }
        else if(ch == 3){
            //ctrl c
            is_running = false;
        }
        else if(ch == KEY_F0 + 1){
            do_menu();
        }
        else if(isOkChar(ch) && !menu_on){
            getyx(stdscr, y, x);
            strings[y][x] = (char)ch;
            x ++;
            addch(ch);
        }
        if( y +1 > maxY) {
            maxY = y + 1;

        }
        getyx(stdscr, y, x);

        
    }
    endwin();
}