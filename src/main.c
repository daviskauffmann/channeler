#include <string.h>

#include "client.h"
#include "server.h"

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--server") == 0 || strcmp(argv[i], "-s") == 0)
        {
            return server_main(argc, argv);
        }
    }

    return client_main(argc, argv);
}
