
const int TWO_HUNDRED_MILLION = 200000000;

void TimerInterrupt (volatile bool &interrupt_out, volatile bool &interrupt_acked)
{
    static unsigned long timer = 0;
    const unsigned long timer_threshold = TWO_HUNDRED_MILLION; // ~ every second for a 200MHz clock

    while(1) {
        timer = 0;
        while (timer++ < timer_threshold);

        timer = 0;

        interrupt_out = true;
        while(!interrupt_acked);
        interrupt_out = false;
    }
}