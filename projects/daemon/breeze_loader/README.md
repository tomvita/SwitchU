# Breeze loader

`main` and `main.npdm` are the exefs of Breeze's User Page loader
(`switch/Breeze/profile_18.zip` → `atmosphere/contents/0100000000001013/exefs.nsp`,
the Atmosphere 1.8+ build). It is nx-hbloader set to start
`sdmc:/switch/breeze/breeze.nro`, falling back to `sdmc:/hbmenu.nro`, and its
npdm carries the Album's program id (`0x010000000000100D`).

`xmake install` copies it to
`atmosphere/contents/0100000000001000/breeze_loader/`, next to the daemon, so a
daemon-only install without the SwitchU menu can still start Breeze. With the
`breeze_first` flag set and no User Page loader installed, the daemon registers
this folder as the Album's code for as long as Breeze runs, the same way the
SwitchU menu borrows the Album slot (see `smi::kBreezeAlbumLoaderDir`).
