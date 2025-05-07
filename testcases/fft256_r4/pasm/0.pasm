epoch <fft256_r4> {
    cell (x=0, y=0) {
        rop <route0r> (slot=0, port=2){
            route(slot=0, option=0, sr=0, source=2, target=0b010000000)
        }

        rop <input_r> (slot=1, port=0){
            dsu(slot=1, port=0, init_addr=0)
            rep(slot=1, port=0, level=0, iter=63, step=1, delay=0)
        }

        rop <input_w> (slot=1, port=2){
            dsu(slot=1, port=2, init_addr=0)
            rep(slot=1, port=2, level=0, iter=63, step=1, delay=0)
        }

        rop <read_val> (slot=2, port=3){
            dsu(slot=2, port=3, init_addr=0)
            rep(slot=2, port=3, level=0, iter=63, step=1, delay=0)
        }
    }

    cell (x=1, y=0) {
        rop <route1wr> (slot=0, port=2){
            route (slot=0, option=0, sr=1, source=1, target=0b0000000000100010)
            route (slot=0, option=0, sr=0, source=1, target=0b010000000)
        }

        rop <write_val> (slot=1, port=2){
            dsu (slot=1, port=2, init_addr=0)
            rep (slot=1, port=2, iter=31, step=1, delay=0)
        }

        rop <write_twid> (slot=5, port=2){
            dsu (slot=5, port=2, init_addr=0)
            rep (slot=5, port=2, iter=31, step=1, delay=0)
        }

        rop <swb> (slot=0, port=0){
            swb (slot=0, option=0, channel=9, source=1, target=9)
            swb (slot=0, option=0, channel=10, source=2, target=10)
            swb (slot=0, option=0, channel=11, source=3, target=11)
            swb (slot=0, option=0, channel=12, source=4, target=12)
            swb (slot=0, option=0, channel=13, source=6, target=13)
            swb (slot=0, option=0, channel=14, source=7, target=14)
            swb (slot=0, option=0, channel=15, source=8, target=15)
            swb (slot=0, option=0, channel=1, source=9, target=1)
            swb (slot=0, option=0, channel=2, source=10, target=2)
            swb (slot=0, option=0, channel=3, source=11, target=3)
            swb (slot=0, option=0, channel=4, source=12, target=4)
        }

        rop <read_d1> (slot=1, port=1){
            fft (slot=1, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <read_d2> (slot=2, port=1){
            fft (slot=2, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <read_d3> (slot=3, port=1){
            fft (slot=3, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <read_d4> (slot=4, port=1){
            fft (slot=4, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }

        rop <read_twid1> (slot=6, port=1){
            fft (slot=6, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=0, delay=0)
        }
        rop <read_twid2> (slot=7, port=1){
            fft (slot=7, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=0, delay=0)
        }
        rop <read_twid3> (slot=8, port=1){
            fft (slot=8, port=1, n_points=256, radix=1, decimation=1, n_bu=0, mode=0, delay=0)
        }

        rop <write_d1> (slot=1, port=0){
            fft (slot=1, port=0, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <write_d2> (slot=2, port=0){
            fft (slot=2, port=0, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <write_d3> (slot=3, port=0){
            fft (slot=3, port=0, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }
        rop <write_d4> (slot=4, port=0){
            fft (slot=4, port=0, n_points=256, radix=1, decimation=1, n_bu=0, mode=1, delay=0)
        }

        rop <bu> (slot=9, port=0){
            bu (slot=9, option=0, radix=1, decimation=1)
        }

        rop <read_res> (slot=1, port=3){
            dsu (slot=1, port=3, init_addr=0)
            rep (slot=1, port=3, level=0, iter=31, step=1, delay=0)
        }
    }

    cell (x=2, y=0) {
        rop <route2w> (slot=0, port=2){
            route (slot=0, option=0, sr=1, source=1, target=0b0000000000000100)
        }

        rop <write_res> (slot=2, port=2){
            dsu (slot=2, port=2, init_addr=0)
            rep (slot=2, port=2, iter=31, step=1, delay=0)
        }

        rop <output_r> (slot=1, port=3){
            dsu (slot=1, port=3, init_addr=0)
            rep (slot=1, port=3, iter=31, step=1, delay=0)
        }

        rop <output_w> (slot=1, port=1){
            dsu (slot=1, port=1, init_addr=0)
            rep (slot=1, port=1, iter=31, step=1, delay=0)
        }
    }
}