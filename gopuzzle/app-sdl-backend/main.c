#include <string.h>

extern char lunaFolderPath[];

int Gopuzzle_Main();

int main(int argc, char *argv[]) {
    strcpy(lunaFolderPath, "../../../moon-example-project/cache/");
    return Gopuzzle_Main();
}