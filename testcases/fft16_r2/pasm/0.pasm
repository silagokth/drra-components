epoch <fft16> {
    cell (x=0, y=0) {
        rop <route0r> (slot=0, port=2){
            route(slot=0, option=0, sr=0, source=2, target=0b010000000)
        }

        rop <input_r> (slot=1, port=0){
            dsu(slot=1, port=0, init_addr=0)
            rep(slot=1, port=0, level=0, iter=2, step=1, delay=0)
        }

        rop <input_w> (slot=1, port=2){
            dsu(slot=1, port=2, init_addr=0)
            rep(slot=1, port=2, level=0, iter=2, step=1, delay=0)
        }

        rop <read_val> (slot=2, port=3){
            dsu(slot=2, port=3, init_addr=0)
            rep(slot=2, port=3, level=0, iter=2, step=1, delay=0)
        }
    }

    cell (x=1, y=0) {
        rop <route1wr> (slot=0, port=2){
            route (slot=0, option=0, sr=1, source=1, target=0b0000000000001010)
            route (slot=0, option=0, sr=0, source=1, target=0b010000000)
        }

        rop <write_val> (slot=1, port=2){
            dsu (slot=1, port=2, init_addr=0)
            rep (slot=1, port=2, iter=1, step=1, delay=0)
        }

        rop <write_twid> (slot=3, port=2){
            dsu (slot=3, port=2, init_addr=0)
            rep (slot=3, port=2, iter=0, step=1, delay=0)
        }

        rop <swb> (slot=0, port=0){
            swb (slot=0, option=0, channel=5, source=2, target=5)
            swb (slot=0, option=0, channel=6, source=3, target=6)
            swb (slot=0, option=0, channel=7, source=1, target=7)
            swb (slot=0, option=0, channel=8, source=5, target=8)
            swb (slot=0, option=0, channel=1, source=7, target=1)
            swb (slot=0, option=0, channel=2, source=8, target=2)
        }

        rop <read_d1> (slot=1, port=1){
            fft (slot=1, port=1, n_points=16, radix=0, n_bu=0, mode=1, delay=0)
        }

        rop <read_d2> (slot=2, port=1){
            fft (slot=2, port=1,  n_points=16, radix=0, n_bu=0, mode=1, delay=0)
        }

        rop <read_twid> (slot=3, port=1){
            fft (slot=3, port=1,  n_points=16, radix=0, n_bu=0, mode=0, delay=0)
        }

        rop <write_d1> (slot=1, port=0){
            fft (slot=1, port=0,  n_points=16, radix=0, n_bu=0, mode=1, delay=0)
        }

        rop <write_d2> (slot=2, port=0){
            fft (slot=2, port=0,  n_points=16, radix=0, n_bu=0, mode=1, delay=0)
        }

        rop <mult> (slot=5, port=0){
            dpu (slot=5, mode=7)
        }

        rop <read_res> (slot=1, port=3){
            dsu (slot=1, port=3, init_addr=0)
            rep (slot=1, port=3, level=0, iter=1, step=1, delay=0)
        }
    }

    cell (x=2, y=0) {
        rop <route2w> (slot=0, port=2){
            route (slot=0, option=0, sr=1, source=1, target=0b0000000000000100)
        }

        rop <write_res> (slot=2, port=2){
            dsu (slot=2, port=2, init_addr=0)
            rep (slot=2, port=2, iter=1, step=1, delay=0)
        }

        rop <output_r> (slot=1, port=3){
            dsu (slot=1, port=3, init_addr=0)
            rep (slot=1, port=3, iter=1, step=1, delay=0)
        }

        rop <output_w> (slot=1, port=1){
            dsu (slot=1, port=1, init_addr=0)
            rep (slot=1, port=1, iter=1, step=1, delay=0)
        }
    }
}