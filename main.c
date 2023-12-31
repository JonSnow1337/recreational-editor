#include <ncurses.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "log.h"
#define _GNU_SOURCE
#define MENU_WIDTH 50
#define MENU_HEIGHT 10

int x = 0;
int y = 0;
char **strings ={0};
bool is_running = true;
int maxY = 1;
char *input_file_name;
int loaded_file_lines = 0;
WINDOW *menu_window;
bool menu_on;
int numOfLines = 10;
int array_expansion = 10;
int charsPerLine = 200;
int binary_data_len = 0;
bool hex_mode = false;
int term_max_y;
int term_max_x;
int y_offset = 0;
int max_y_offset = 0;
int new_lines = 0;
bool isOkChar(char c){
    return (c >= 32 || c <127) && c != KEY_BACKSPACE;
}

void disable_flow_control() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_iflag &= ~(IXON | IXOFF);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void replace_percent(char *line){
}
void expand_array(int expand_num){
    strings = (char **)realloc(strings, sizeof(char *) * (numOfLines + expand_num));
    for(int i = numOfLines; i < numOfLines + expand_num; i++){
        strings[i] = (char*) malloc(sizeof(char) * charsPerLine);
        memset(strings[i], 0, charsPerLine);
    }
    numOfLines += expand_num;
}


void loadFile(char *fileName){
    log_it(DEVEL, "%s", "hello there");

    FILE *fileptr;
    ssize_t read;
    size_t len = 0;
    char *line;

    fileptr = fopen(fileName, "r");
    int i = 0;
    int j = 0;
    if(hex_mode){
        fileptr = fopen(fileName, "rb");
        int c;
        while((c = getc(fileptr)) != EOF){
            // printf("i:%d j:%d ", i,j );
            log_it(DEVEL, "%d", i);

            if( i == numOfLines -1){
                log_it(DEVEL, "%s", "expanding array");
                expand_array(10);
            }
            strings[i][j] = (unsigned char)c;
            binary_data_len ++;
            j++;
            if(j >= charsPerLine){
                i ++;
                loaded_file_lines ++;
            }
            if(i >= numOfLines -1){
                return;
            }
        }

        return;
    }
    while ((read = getline(&line, &len, fileptr)) != -1) {
        if(loaded_file_lines == numOfLines){
            expand_array(10);
        }
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
    for (int i = 0; i < loaded_file_lines + new_lines + 1; i++){
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

int send_packet(char *buf){
    int fd;
    struct sockaddr_in targetAddr;
    buf = "small test";
    fd = socket(AF_INET,SOCK_DGRAM,0);
    if (fd < 0){
        return -1;
    }
    memset(&targetAddr,0, sizeof(targetAddr));
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(8080);
    targetAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    sendto(fd,buf,strlen(buf),0, (struct sockaddr*)&targetAddr, sizeof(targetAddr) );
    close(fd);
    return 0;

}

void do_menu(){
    int oldX = x;
    int oldY = y;
    if(menu_window == NULL){
        noecho();
        menu_window = create_newwin(MENU_HEIGHT,MENU_WIDTH,y,x);
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
                send_packet(NULL);
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
void scroll_down(int y_offset){
    wscrl(stdscr,1);
    //-2 because for some reason cruses wont print at the last line
    mvprintw(term_max_y-2,0,"%s",strings[y + y_offset]);
    move(term_max_y -2, 0);
    refresh();
}

void scroll_up(int y_offset){
    if(y_offset < 0){
        return;
        
    }
    wscrl(stdscr, -1);
    mvprintw(0,0,"%s",strings[y_offset]);
    refresh();
}
int main(int argc, char *argv[]){

    disable_flow_control();
    strings = (char**)malloc(numOfLines * sizeof(char*));
    for (int i = 0; i < numOfLines; i++){
        strings[i] = (char*)malloc(charsPerLine * sizeof(char));
        memset(strings[i], 0, charsPerLine);
    }

    initscr();
    curs_set(true);
    keypad(stdscr,true);
    noecho();
    scrollok(stdscr, true);
    getmaxyx(stdscr, term_max_y,term_max_x);
    if(argc > 1) {
        input_file_name = argv[1];
        //printf("%s", input_file_name);
        printf("hello\n");
        loadFile(input_file_name);
        for (int i = 0 ; i < term_max_y -1; i++)
        {
            if(hex_mode){

                for(int j = 0; j < binary_data_len; j++){
                    printw("%02X", (unsigned char)strings[i][j]);
                    printw(" ");
                }
            }
            else{
                printw("%s",strings[i]);

            }

        }
    }
    while (is_running) {

        if ( y + y_offset  == numOfLines){
            //we are out of memory and must allocate more
            expand_array(10);
        }
        move(y,x);
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
            if(y   >= term_max_y - 2 ){
                y_offset ++;
                scroll_down(y_offset);
            }else{
                y ++;
                // if(y_offset >= max_y_offset){
                //     //we are entering uncharted teritory
                //     //TODO test if this works when scrolling below text
                //     strings[y][x] = '\n';
                //     for(int i =0; i < x-1; i++){
                //         if(strings[y][i] == 0){
                //             strings[y][i] = ' ';
                //         }
                //     }
                //     new_lines++;
                // }
                move(y,x);      
            }
        }
        else if(ch == KEY_UP){
            if( y == 0 ){
                y_offset --;
                scroll_up(y_offset);

                if(y_offset < 0){
                    y_offset = 0;
                }
            }else{
                y --;
            }
            move(y,x);

        }
        else if(ch == '\n') {
            //check if the line already contains a new line, in this case do not add anything
            if(strchr(strings[y], '\n') == NULL){
                strings[y][x] = '\n';
                strings[y][x+1] = '\0';
                new_lines ++;
            }
            x = 0;
            y ++;
            move(y,x);

        }
        else if(ch == KEY_BACKSPACE) {
            delch();
            x --;
            move(y,x);
            //TODO check if line empty and remove it
            //new_lines --;
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

            strings[y + y_offset][x] = (char)ch;
            x ++;
            addch(ch);
        }
        if( y +1 > maxY) {
            maxY = y + 1;

        }
        if(y_offset > max_y_offset){
            max_y_offset = y_offset;
        }
        log_it(DEVEL, "y offset : %d", y_offset);
        log_it(DEVEL, " y :%d", y);
        log_it(DEVEL, "%s", strings[y + y_offset]);


        getyx(stdscr, y, x);
    }
    endwin();
}