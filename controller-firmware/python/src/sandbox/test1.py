






for i in range(32):
    print(f"4 bit value: {i%16}\tstep: {1}\t8 bit value: {i%16 << 4}\tstep: {1<<4}")