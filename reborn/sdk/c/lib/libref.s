; ****************************************************************************
; libref.s - the library's reference for cc65's driver kernels (TGI's: tgi-kernel.s puts it in a driver's header),
; as each of cc65's targets has one: a loadable driver would find the library's functions through it.  The
; Hydra's drivers are static (hydra_tgi), so it's only there to be linked.

            .export     tgi_libref
            .import     _exit

tgi_libref  := _exit
