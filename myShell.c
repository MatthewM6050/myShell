// @author Matthew Melendez 
// When testing egrep, make sure that the parameter is not between quotation marks or apostrophes.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_ARGUMENTS 10 // Maximum number of arguments for each command
#define MAX_CMD_LEN 100 // Maximum length of command

void pipe_execute(char **commands, int n_commands, char *outfile); // Function declaration for pipe_execute()

int main() 
{
    char command[MAX_CMD_LEN];   // Input command buffer
    char *commands[MAX_ARGUMENTS];    // Array of command arguments
    char *outfile = NULL;    // Output file name, if specified
    int n_commands = 0;          // Number of individual commands

    // Start an infinite loop to keep taking commands
    while (1) 
    {
        printf("MyShPrompt> ");   // Prompt user for input
        fgets(command, MAX_CMD_LEN, stdin); // Read input from stdin
        command[strcspn(command, "\n")] = '\0'; // Remove the trailing newline from the input

        // Check if output file is specified
        if (strstr(command, ">") != NULL) 
        {
            outfile = strtok(command, ">"); // Get the part of the command before the ">" character
            outfile = strtok(NULL, " "); // Get the next part of the command, which should be the output file name
            outfile[strcspn(outfile, " ")] = '\0'; // Remove the trailing whitespace from the output file name
        }

        // Split command into individual commands using the "|" character as the delimiter
        char *token = strtok(command, "|");
        while (token != NULL && n_commands < MAX_ARGUMENTS) // As long as there are still tokens left and we haven't reached the maximum number of arguments
        {
            commands[n_commands++] = token; // Add the current token to the array of command arguments
            token = strtok(NULL, "|"); // Get the next token
        }

        // If there is at least one command, execute the pipeline
        if (n_commands > 0) 
        {
            pipe_execute(commands, n_commands, outfile); // This is a placeholder function call that presumably executes the pipeline of commands
        }

        // Reset commands and n_commands for next command
        memset(commands, 0, sizeof(commands)); // Set all the elements of the commands array to 0
        n_commands = 0; // Reset the number of commands
        outfile = NULL; // Reset the output file name
    }

    return 0; // End the program
}


void pipe_execute(char **commands, int n_commands, char *outfile) 
{
    // Variables
    int i, fd_input, fd_output;  // File descriptors for input and output
    int fd[2];  // Array of file descriptors for the pipe
    pid_t pid;  // Process ID of the child process
    char buffer[1024];
    int bytes_read;

    fd_input = 0;  // Initial input will come from stdin

    // Loop through each command in pipeline
    for (i = 0; i < n_commands; i++) 
    {
        // Pipe Error Handling
        if (pipe(fd) < 0) {  // Create pipe
            perror("pipe error");
            exit(EXIT_FAILURE);
        }

        pid = fork();  // Create child process
        // Fork Error Handling
        if (pid < 0) 
        {
            perror("fork error");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) // Child process
        {
            close(fd[0]);  // Close read end of pipe

            // Redirect input to previous pipe read end
            if (fd_input != 0) 
            {
                // dup2 Error Handling for input end
                if (dup2(fd_input, STDIN_FILENO) < 0) 
                {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                close(fd_input);
            }

            // Redirect output to current pipe write end
            if (fd[1] != 1) 
            {
                // dup2 Error Handling for input end
                if (dup2(fd[1], STDOUT_FILENO) < 0) 
                {
                    perror("dup2 error");
                    exit(EXIT_FAILURE);
                }
                close(fd[1]);
            }

            // Execute command
            char *args[MAX_ARGUMENTS];
            int n_args = 0;
            char *token = strtok(commands[i], " ");

            while (token != NULL && n_args < MAX_ARGUMENTS) 
            {
                args[n_args++] = token;
                token = strtok(NULL, " ");
            }
            args[n_args] = NULL;

            if (execvp(args[0], args) < 0) // Execute command
            {
                perror("execvp error");
                exit(EXIT_FAILURE);
            }
        }

        close(fd[1]);  // Close write end of pipe

        fd_input = fd[0];  // Set input for next command to current pipe read end
        waitpid(pid, NULL, 0);  // Wait for child process to complete
    }

    // Redirect output to file if specified
    if (outfile != NULL) 
    {
        fd_output = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        
        // Output Error Handling
        if (fd_output < 0) 
        {  
            perror("open error");
            exit(EXIT_FAILURE);
        }

        // dup2 Error Handling for output end
        if (dup2(fd_output, STDOUT_FILENO) < 0) 
        {
            perror("dup2 error");
            exit(EXIT_FAILURE);
        }

        close(fd_output);
    }

    // Read remaining output from last command in pipeline
    while ((bytes_read = read(fd_input, buffer, sizeof(buffer))) > 0) 
    {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    close(fd_input);
}
