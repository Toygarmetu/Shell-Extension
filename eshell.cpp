#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>

#include "parser.h"

using namespace std;

int main() {

    string input;
    parsed_input pinput;

    pid_t pid;
    int status;
    bool quit = false;

    while (!quit) {
        cout << "/> ";
        getline(cin, input);

        istringstream iss(input);
        string command;

        vector<pid_t> children; 

        while (getline(iss, command, ';')) {
            istringstream iss2(command);
            string subcommand;

            while (getline(iss2, subcommand, ',')) {
                if (!parse_line(const_cast<char*>(subcommand.c_str()), &pinput)) {
                    cerr << "Invalid input" << endl;
                    continue;
                }
                //pretty_print(&pinput);

                if (pinput.inputs[0].type == INPUT_TYPE_SUBSHELL) {
                // Handle subshell execution
                pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Child process
                    execl("/bin/sh", "sh", "-c", pinput.inputs[0].data.subshell, (char *)NULL);
                    perror("execl"); // execl returns only on error
                    exit(EXIT_FAILURE);
                } else {
                    // Parent process
                    waitpid(pid, &status, 0);
                }
            } else {

                if (pinput.num_inputs == 0) {
                    continue;
                }

                if (strcmp(pinput.inputs[0].data.cmd.args[0], "quit") == 0) {
                    quit = true; 
                    break;
                }

                int n = pinput.num_inputs; 
                int pipefd[n - 1][2]; 

                for (int i = 0; i < n; i++) {
                    if (i < n - 1) {
                        if (pipe(pipefd[i]) < 0) {
                            perror("pipe");
                            exit(EXIT_FAILURE);
                        }
                    }

                    pid = fork();

                    if (pid < 0) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    } 
                    else if (pid == 0) {
                        if (i > 0) {
                            if (dup2(pipefd[i - 1][0], 0) < 0) {
                                perror("dup2");
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (i < n - 1) {
                            if (dup2(pipefd[i][1], 1) < 0) {
                                perror("dup2");
                                exit(EXIT_FAILURE);
                            }
                        }

                        for (int j = 0; j < i; j++) {
                            close(pipefd[j][0]);
                            close(pipefd[j][1]);
                        }

                        single_input* sput = &pinput.inputs[i];
                        char **para = sput->data.cmd.args;
                        if (execvp(para[0], para) < 0) {
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else {
                        if (i > 0) {
                            close(pipefd[i - 1][0]);
                        }
                        if (i < n - 1) {
                            close(pipefd[i][1]);
                        }
                        children.push_back(pid); 
                    }
                }
            }
                free_parsed_input(&pinput);
            }
            if (quit) break;

            for (size_t i = 0; i < children.size(); i++) {
                waitpid(children[i], &status, 0);
            }

            children.clear(); 
        }
    }
    return 0;
}
