#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#define ANCIN ("/dev/tsin1")
#define ANCOUT ("anc.dump")

int main()
{
        int f_in;
        int f_out;
        char buf[256];
        printf("ANC dump\n");

        f_in = open(ANCIN, O_RDONLY);
        assert(f_in>0);
        
        f_out = open(ANCOUT, O_RDWR|O_CREAT, 0664); // ign
        assert(f_out>0);

        for(int i=0; i<256; i++){
                int ret = read(f_in, buf, sizeof(buf));
        }

        time_t start = time(NULL);

        while(start+60 > time(NULL)){
                int ret = read(f_in, buf, sizeof(buf));
                write(f_out, &ret, sizeof(ret));
                write(f_out, buf, ret);
        }

        close(f_in);
        close(f_out);
}
