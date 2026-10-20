int clock_case, clock_calls;

void fake_clock(unsigned short *v) {
    clock_calls++;
    v[0] = 2026;
    v[1] = 1;
    v[3] = 2;
    v[4] = 3;
    v[5] = 4;
    v[6] = 5;
    if (clock_case == 0)
        v[0] = 0;
    if (clock_case == 1)
        v[0] = 10000;
    if (clock_case == 2)
        v[1] = 0;
    if (clock_case == 3)
        v[1] = 13;
    if (clock_case == 4)
        v[3] = 0;
    if (clock_case == 5)
        v[3] = 32;
    if (clock_case == 6) {
        v[1] = 4;
        v[3] = 31;
    }
    if (clock_case == 7) {
        v[0] = 1900;
        v[1] = 2;
        v[3] = 29;
    }
    if (clock_case == 8)
        v[4] = 24;
    if (clock_case == 9)
        v[5] = 60;
    if (clock_case == 10)
        v[6] = 60;
    if (clock_case == 11) {
        v[0] = 2000;
        v[1] = 2;
        v[3] = 29;
    }
    if (clock_case == 12) {
        v[0] = 9999;
        v[1] = 12;
        v[3] = 31;
    }
    if (clock_case == 13) {
        v[0] = 1;
        v[1] = 1;
        v[3] = 1;
    }
}

void unavailable_clock(unsigned short *v) {
    clock_calls++;
}

int text_matches(int start, int length, char *text) {
    int i;
    for (i = 0; text[i]; i++)
        if (i >= length || pp_char(start, i) != text[i])
            return 0;
    return i == length;
}

int main(void) {
    int i, saved;
    char *date;
    for (i = 0; i < 15; i++) {
        clock_case = i;
        clock_calls = 0;
        pp_generated = 0;
        pp_clock_ready = 0;
        failed = 0;
        if (i == 14)
            pp_clock_read(unavailable_clock);
        else
            pp_clock_read(fake_clock);
        if (failed)
            return 1;
        date = "\"Jan  1 1970\"";
        if (i == 11)
            date = "\"Feb 29 2000\"";
        if (i == 12)
            date = "\"Dec 31 9999\"";
        if (i == 13)
            date = "\"Jan  1 0001\"";
        if (!text_matches(pp_date_start, pp_date_length, date))
            return 2;
        if (!text_matches(pp_time_start, pp_time_length,
                          i < 11 || i == 14 ? "\"00:00:00\"" : "\"03:04:05\""))
            return 3;
        saved = pp_generated;
        pp_clock_read(fake_clock);
        if (clock_calls != 1 || pp_generated != saved || !pp_clock_ready)
            return 4;
    }
    return 0;
}
