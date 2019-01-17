/* For this mini-project I write a "baby" command shell,
which have simplyfied shell function, ex. run common
command such as cd, ls, exit... And we can set and export
variables. We can also use redirection*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <unistd.h> 
#include <string>
#include <vector>
#include <linux/limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <map>
using namespace std;

extern char ** environ;
//this map stores variable names and values in step3
map<string, string> myMap;

/* This function put the environment variable name
 * and value into the map, so that we can use these variables */
void initEnvi() {
    size_t i = 0;
    string str;
    string k;
    string v;
    while(environ[i] != NULL) {
        str = environ[i++];
        size_t eq = str.find("=");
        if(eq == string::npos)
            return;
        k = str.substr(0, eq - 1); // k is variable's name
        v = str.substr(eq + 1, str.length());//v is variable's value
        myMap[k] = v;
    }
}

/* This function takes 1 argument which is 
 * the variable's name. This function checks whether a variable name 
 * is valid or not. A variable name must be a 
 * combination of letters (case sensitive), underscores,
 * and numbers, */
bool validKey(string key) {
    if(key.length() == 0 || key == "")
        return false;
    for(size_t i = 0; i < key.length(); i++) {
        if(!isalnum(key[i]) && key[i] != '_')
            return false;
    }
    return true;
}
/* This function takes 1 argument which is the argument 
 * name and value. This function implements set. It sets the variable
 * var to the string on the rest of the command line, 
 * The shell remembers this value, and make use of it 
 * in future $ evaluations, however, it doesn't not be 
 * placed in the environment for other programs */
void set(string kvp) {
    size_t token1 = kvp.find(" ");
    size_t token2 = kvp.find(" ", token1 + 1);
    if(token1 == string::npos){
        perror("invalid set");
        return;
    }
    string key = kvp.substr(token1 + 1, token2 - token1 - 1);
    string value = token2 == string::npos ? "" : kvp.substr(token2 + 1, kvp.length());
    if(!validKey(key)) {
      perror("invalid variable name");
      return; 
    }
    myMap[key] = value;
}

/* This function takes 1 arguemt which is the variable, 
 * This function substitutes the variable name for its value
 * in command line, ex. after setting "a" to "README" and 
 * export a, cat $a would be cat README */
string substi(string var) {
    size_t cur = 0;
    size_t next = var.size();
    string value = "";
    string sub = "";
    while(cur < var.size()) {
        if(var[cur] == '$') {
            next = var.find('$', cur + 1); //find next "$"
            if(next != string::npos) {
                sub = var.substr(cur + 1, next - cur - 1); //split string between 2 "$"
                if(myMap.find(sub) != myMap.end())
                    value += myMap[sub];
                else
                    value = value + "$" + sub;
            } else {
                sub = var.substr(cur + 1, var.size());
                if(myMap.find(sub) != myMap.end()) //find weather variable exists
                    value += myMap[sub];
          else
                value = value + "$" + sub;
                break;
            }
            cur = next;
            continue;
        }
        value += var[cur++];
    }
    return value;
}

/* This function contains 1 argument, which is the variable' name
 * which is This function implements export, it puts
 * the current value of var into the environment
 * for other programs. */
void myExport(string str) {
    size_t p = str.find(" ");//find variable's name
    if(p == string::npos)
        return;
    string sub = str.substr(p + 1, str.size()); //get variable's name
    string key = substi(sub);
    if(myMap.find(key) == myMap.end())
        return;
    string value = myMap[key];//find variable's value
    char *kbuff = new char[key.size() + 1 ];
    kbuff[key.copy(kbuff, key.size() + 1 )] = '\0';
    char *vbuff = new char[value.size() + 1 ];
    vbuff[value.copy(vbuff, value.size() + 1 )] = '\0';
    setenv(kbuff, vbuff, 1);
    delete[] kbuff;
    delete[] vbuff;
}

/* This function takes 3 arguments, the pathLine is PATH, 
 * token should be ":" and the vector contains splited pathes.
 *This function split the PATH by ":" */
void split(const string& pathLine, string token,vector<string>& pathes) {
  size_t i = 0;
  size_t p = pathLine.find(token);// p is the position of ":"
  while (p != string::npos) {
    pathes.push_back(pathLine.substr(i, p-i));
    i = ++p;
    p = pathLine.find(token, p);//find next ":"

    if (p == string::npos)
       pathes.push_back(pathLine.substr(i, pathLine.length()));
  }
}

/* This function takes 4 arguments, the string file is the file name
 * the vector caontains arguments the newargv is used in execve(),
 * argc is the number of arguments. 
 * For details, please see man page open(), dup2, close().
 * This function handles different redirection --
 * standard input redirection, standard ouput redirection
 * standard error redirection */
void redirectExe(string file, char** newargv, int argc, int num) {
  char *fileBuff = new char[file.size() + 1 ];
  fileBuff[file.copy(fileBuff, file.size() + 1 )] = '\0';
  const char * token;
  int fd = -1;
  if(num == 0)
    token = "<";
  if(num == 1)
    token = ">";  
  if(num == 2)
    token = "2>";
  for(int i = 0; i < argc; i++) {
    if(strcmp(token, newargv[i]) == 0) {
      newargv[i] = NULL;
    }
  }
  if(num == 0) {
    fd = open(fileBuff, O_RDONLY);
  }
  if(num == 1) {
    fd = open(fileBuff, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  }
  if(num == 2) {
    fd = open(fileBuff, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  }
  int newfd = dup2(fd, num);
  if(newfd == -1) {
    perror("invalid dup");
    delete[] fileBuff;
    return;
  }
  execve(newargv[0], newargv, environ);
  close(fd);
  delete[] fileBuff;
}

/* This function taks 3 arguments, the vector caontains arguments
 *the newargv is used in execve(), argc is the number of arguments
 *This function implements input redirection(<) 
 * and output redirection (>), the filenames near the
 * redirection sign are split by white space. If we 
 * find the command is a redirection command it will
 * excute the corespongding redierction. */
void redirect(vector<string>& argv, char ** newargv, int argc) {
  int in = -1;
  int out = -1;
  int er = -1;
  for(size_t i = argv.size() - 1; i >= 0; i--) {
    if(in == -1 && argv[i] == "<") {//this is an input redirection
      if(i < 1 || i == argv.size() - 1)
       return;
      in = i;//choose the last valid input redirection argument
    }
    if(out == -1 && argv[i] == ">") {//this is a outnput redirection
      if(i <= 1 || i == argv.size() - 1)
        return;
      out = i;//choose the last valid output redirection argument
    }
    if(er == -1 && argv[i] == "2>") {//this is a error redirection
      if(i <= 1 || i == argv.size() - 1)
        return;
      er = i;//choose the last valid error redirection argument
    }
    if(i == 0)
      break;
  }
  if(in == -1 && out == -1 && er == -1) // if we cannot find valid ">", "<" or "2>"
    return;
  if(in != -1) {
    string inr = argv[in + 1];
    redirectExe(inr, newargv, argc, 0);
  }
  if(out != -1) {
    string outr = argv[out + 1];
    redirectExe(outr, newargv, argc, 1);
  }
  if(er != -1) {
    string err = argv[er + 1];
    redirectExe(err, newargv, argc, 2);
  }

}

/* This function takes 1 argument which is the program 
 *  choosing by the user.This function searches the program 
 *  in the specified directory. For details, please see man page
 *  opendir(), readir()... 
 *  It first splite the input argument into path
 *  and program, ex. "/bin/cat" -> "/bin" "cat". Then it 
 *  modifies the path into direct path and search the program
 *  in the direct path. If the program is found, the shell
 *  execute it. If not, my shell print Command commandName 
 *  not found. */
string searchWithS(string& program) {
  DIR *dir;
  struct dirent *file;
  char * resolved_path = NULL;
  char * progBuf = new char[program.size() + 1];
  char pathbuf[PATH_MAX];
  progBuf[program.copy(progBuf,program.size() + 1)] = '\0';//trasfer string into char*
  resolved_path = realpath(progBuf, pathbuf);//get direct path
  delete[] progBuf;
  if(resolved_path == NULL) {
    perror("invalid path");
    return "";
  } 
  string res = resolved_path;
  size_t token = res.find_last_of("/");
  string command = res.substr(token + 1, res.size());
  string path = res.substr(0, token);
  if((dir = opendir(path.c_str()))==NULL){
    perror("cannot open the file!\n");
    cout << "Command "<< command <<" not found" <<endl;
    return "";
  }
  while ((file = readdir(dir)) != NULL){
      if(file->d_name == command) {//if we find this command is valid in this path we can use the command
        if(closedir(dir)!=0){
         cout << "Command "<< command <<" not found" <<endl;
         perror("fail closing the file!\n");
         break;
        }
      return res;
    }
  }
  if(closedir(dir)!=0){
    cout << "Command "<< command <<" not found" <<endl;
    perror("fail closing the file!\n");
    return "";
  }
  cout << "Command "<< command <<" not found" <<endl;
  return "";
}

/* This function takes 1 argument which is the program 
 *  choosing by the user.This function search the program
 *in PATH, becausethe user doesn't type a path with '/'.
 *If the program is found, execute it.  If not, my shell print Command 
 *commandName not found
 */
string searchWithoutS(string& program) {
  DIR *dir;
  struct dirent *file;
  string path = getenv("PATH");
  vector<string> pathes;
  split(path, ":", pathes);// split the PATH by ":" to get valid pathes
  for(size_t i = 0; i < pathes.size(); i++) {
      if((dir = opendir(pathes[i].c_str())) == NULL){
        perror("cannot open the file!\n");
        continue;
      }
      while ((file = readdir(dir)) != NULL){
          if(file->d_name == program) {
             string res = pathes[i] + "/" + program;
          if(closedir(dir)!=0){
             perror("fail closing the file!\n");
             break;
          }
          return res;
        }
    }
  if(closedir(dir)!=0) {
      perror("fail closing the file!\n");
      continue;
    }
  }
  cout << "Command "<< program <<" not found" <<endl;
  return "";  
}

/* This function takes 1 argument which is the program 
 *  choosing by the user.
 * This function  searches weather the command  
 * exits in the specific file. If the command contains
 * '/', it only look in the specified directory, else
 * it search the PATH environment variable for commands */
string searchFile(string& program) {
  size_t found = program.find("/");
  if(found != string::npos){
    return searchWithS(program); //if it contains '\', we should take it as a whole valid path
  } else {
    return searchWithoutS(program); //else we should search PATH
  }
}


/* This function takes 2 arguments, the string is the input 
 * command line from user and the vector saves input arguments
 * This function execute the command.For detials please see man 
 * page fork(), execve()... Firstly it searches weather the
 * command is valid or not. If it is valid, we can excute the command. */
void process(string commandLine, vector<string>& argv) {
  string argv0 = searchFile(argv[0]);// argument 0 is the real command, others are arguments
  if(argv0 == "") {
    return;
  }
  int argc = argv.size();
  pid_t cpid, w;
  int status;
  char ** newargv = new char * [argc + 1]; 
  char *argv0Buf = new char[argv0.size() + 1 ]; // we should put modified argv0 into the valid argv array
  argv0Buf[argv0.copy(argv0Buf, argv0.size() + 1 )] = '\0';
  newargv[0] = argv0Buf;
  for(size_t i = 1; i<argv.size(); i++){
        argv0Buf = new char[argv[i].size() + 1];
        int l = argv[i].copy(argv0Buf,argv[i].size() + 1 );
        argv0Buf[l] = '\0';
        newargv[i] = argv0Buf;
    }
  newargv[argv.size()] = NULL;// the final argument should be NULL by man page

  //run the specified command---fork, execve, and waitpid.
  cpid = fork();
  if (cpid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
  }

  if (cpid == 0) {            
      if (commandLine.find("<") != string::npos || commandLine.find(">") != string::npos) {
        redirect(argv, newargv, argc); // if we find this command is redirection, we should handle it in redirect function
      } else
        execve(newargv[0], newargv, environ);

  } else {                    
      do {
          w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
          if (w == -1) {
              perror("waitpid");
          for(size_t i = 0; i<=argv.size(); i++){
           delete[] newargv[i];
       }
      delete[] newargv;
              return;
          }

          if (WIFEXITED(status)) {
              printf("Program exited with status %d\n", WEXITSTATUS(status));
          } else if (WIFSIGNALED(status)) {
              printf("Program was killed by signal %d\n", WTERMSIG(status));
          }
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
   }
  for(size_t i = 0; i<=argv.size(); i++){
        delete[] newargv[i];
    }

  delete[] newargv;

}

/* This function takes 1 argument, which contains input arguments.
 * This function implements "cd" command, if there is
 * no argument, it is invalid, else if there are several 
 * arguments, it only takes the first argument. */
void cdHelper(vector<string>& argv) {
  if(argv.size() <= 1) {
    fprintf(stderr, "invalid cd");
  } else {
    char * argvBuf = new char[argv[1].size() + 1];
    argvBuf[argv[1].copy(argvBuf,argv[1].size() + 1)] = '\0'; // trasfer string to char *
    if(chdir(argvBuf) != 0)
      perror("invalid");
    delete[] argvBuf;
  }
}

/* This function takes 2 arguments. String input 
 * is the command user types, and the vector 
 * contains the arguments after spliting.
 *This function handles different commands. 
 * If the command is "cd", "set" or "export", this 
 * function would use corespoding function to handle
 * these cases, else it would process the common 
 * command. */
void execute(string input, vector<string>& argv) {
  if(argv.size() == 0 || argv[0] == "")
    return;
  if(argv[0] == "cd") {
    cdHelper(argv); //handle "cd" case
  } else if(argv[0] == "set") {
    set(input); //handle "set" case
  } else if(argv[0] == "export") {
    myExport(input); //handle "input" case
  } else {
    process(input, argv); // handle other cases
  }
}

/* The shell reads one line of input (from stdin) 
 * which is the name of a command. If the user
 * types the command "exit", or "EOF" is encountered 
 * reading from stdin, then
 * the shell exits */
string readLine(void) {
  string commandLine;
  if(!getline(cin, commandLine).good()) // this checks eof and other bad cases
    exit(EXIT_SUCCESS);
  if(commandLine == "exit")
    exit(EXIT_SUCCESS);
  return commandLine;
}

/* This function takes 1 argument str which is the string to 
 * be modified.This is a helper function of splitLine function and it 
 * will replace "\ " with " ".*/
string modifyStr(string& str) {
  size_t found = str.find("\\ ");
  while (found != string::npos) {
    str.erase(found, 1);
    found = str.find("\\ ", found); // find after last time's search
  }
  return str;
}

/* This function takes 2 arguments, the commandLine
 * is the input command, the vector saves the path 
 * This function parse the command line into valid
 * command and arguments. In this project we parse 
 * commands by whitespaces. However if there is a '/'
 * before the whitespace, we would not use this to 
 * splite the command. Instead, I will take '/ ' as a 
 * common character ' 'in the argument. We put valid 
 * commands into a vector, in order to execute commands later. */
void splitLine(string commandLine, vector<string>& path){
  int j = 0;
  size_t start = commandLine.find_first_not_of(" "); //skip head whitespace
  size_t end = commandLine.find_last_not_of(" ");//skip ending whitespace
  if(start != string::npos)
    commandLine = commandLine.substr(start);
  if(end != string::npos)
    commandLine = commandLine.substr(0, end + 1);
    commandLine = commandLine + " ";
  for(size_t i = 0; i < commandLine.length(); i++) {
      if(commandLine[i] == ' ' && (i == commandLine.length() - 1 || i == 0 || commandLine[i - 1] != '\\' )) {
       string subStr = commandLine.substr(j, i - j); //get input argument spliting by " "
       string validStr = modifyStr(subStr); // replace "/ " with " "
       if( validStr.find("$") != string::npos)
        validStr = substi(validStr); ////replace variable name with its value when there is "$"
       if(subStr != "") //if there are whitespaces between 2 arguments don't take it as valid arguments
        path.push_back(validStr);
       j = i + 1; //j should be the index after the " " that  we just found
     }
  }
}

/* In the loop, we get command line,
 * parse command line to valid command and 
 * then run the command. it always print a 
 * prompt ("myShell $") to get command 
 */
void loop(void) {
  do {
  string input;
  string inputCopy; //string input can be changed in splitLine, so we should save it to use it in execute
  char inputBuf[PATH_MAX];
  getcwd(inputBuf, sizeof(inputBuf)); // get path name
  cout << "myShell" << ":" << inputBuf << " $";
  vector<string> argv;
  input = readLine();
  inputCopy = input;
  splitLine(input, argv); 
  execute(inputCopy,argv);
  } while (1);
}

int main() {
//save environment variables and values into map
  initEnvi();
//keep looping to search entered commands
  loop();
  return EXIT_SUCCESS;
}
