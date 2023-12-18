#include "matrix_drv.h"
#include "letter.h"

int matrix_thread_runner(void* data) {
    static u64 last_boot = 0, last_used = 0;
    char* ledState = kmalloc(8, GFP_KERNEL);

    while(!kthread_should_stop()) {
        if(matrix_mode == MAT_CPU) {
            u64 curr_used = 0, cpus = 0;
            int i;

            for_each_possible_cpu(i) {
                curr_used += kcpustat_cpu(i).cpustat[CPUTIME_USER]
			  +  kcpustat_cpu(i).cpustat[CPUTIME_NICE]
			  +  kcpustat_cpu(i).cpustat[CPUTIME_SYSTEM]
			  +  kcpustat_cpu(i).cpustat[CPUTIME_SOFTIRQ]
			  +  kcpustat_cpu(i).cpustat[CPUTIME_IRQ];

                cpus++;
            }

            u64 curr_boot = ktime_get_boottime_ns() * cpus;
            s32 diff_boot = (curr_boot - last_boot) >> 16;
            s32 diff_used = (curr_used - last_used) >> 16;
            last_boot = curr_boot;
            last_used = curr_used;
            s64 usage = 0;
            if (diff_boot <= 0 || diff_used < 0)
		        usage = 0;
	        else if (diff_used >= diff_boot)
		        usage = U64_MAX;
	        else
		        usage = (100 * diff_used / diff_boot);

            usage = ((usage * 64) / 100);

            *(u64*)ledState = 0;
            while(usage-- > 0) {
                *(u64*)ledState |= 1;
                *(u64*)ledState <<= 1;
            }
            *(u64*)ledState >>= 1;

            matrix_try_send(MATRIX_IMAGE_RQ, 0, ledState, 8);

            msleep_interruptible(100);
        }
        else if(matrix_mode == MAT_TEXT) {
            static u32 rows[8] = {  };
            static char* textPtr = matrix_text;
            static u32 rowPtr = 8;

            while(rowPtr < 16 && *textPtr) {
                int crow = 0;
                const char* rune = chmap[max(min(*textPtr - 32, 112), 0)];
                u32 crowPtr = rowPtr;
                while(*rune) {
                    if(*rune == '1')
                        rows[crow] |= (1 << (31 - crowPtr));
                    else if(*rune == ',') {
                        crow++;
                        crowPtr = rowPtr;
                    }
                    if(*rune == '1' || *rune == '0')
                        crowPtr++;
                    rune++;
                }
                rowPtr = crowPtr + 1;
                if(!*++textPtr)
                    textPtr = matrix_text;
            }

            for(int i = 0; i < 8; i++) {
                ledState[i] = (rows[i] & 0xff000000) >> 24;
                rows[i] <<= 1;
            }

            if(rowPtr > 8)
                rowPtr--;

            matrix_try_send(MATRIX_IMAGE_RQ, 0, ledState, 8);
            
            msleep_interruptible(matrix_text_speed);
        }
    }

    kfree(ledState);
    return 0;
}

MODULE_VERSION("0.1");
MODULE_DESCRIPTION("LED 8x8 Matrix driver");
MODULE_AUTHOR("s188741");
MODULE_LICENSE("GPL");