/*
GRUP 57
1. ÖĞRETİM C GRUBU
B221210069 Seray Eğe 
B221210015 Yüsra Şengün
*/

#ifndef PROGRAM_H
#define PROGRAM_H

//---LIBRARIES---//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

//---SABITLER---//
#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
#define KNRM "\x1B[0m"
#define KWHT "\x1B[37m"
#define KBLU "\x1B[34m"

void Prompt();
void parse_command(char *command, char **args);
int execute_args(char **args);
int new_process(char **args);
void dosya_input(char *command[], char *input_file); 
void dosya_output(char *command[], char *output_file);
void pipe_commands(char *cmd1[], char *cmd2[]);
int own_help(char **args);
int own_cd(char **args);
int own_env(char **args);
int own_exit(char **args);
int arkaPlandaCalistir(char **args);
void process_pipeline(char *command);
int quit_komut(pid_t child_pids[], int *child_count);
void sig_chld(int signo);
void handle_redirection(char **args);
void parse_pipelines_and_execute(char *command);
void parse_multiple_commands(char *command);
void execute_command(char *input);

#endif
