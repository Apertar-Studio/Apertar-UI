# liblj92 decoder

Vendored from cgohlke/imagecodecs commit
`07b03834908db1ad41209bc175c495ff08cb9746`, directory `3rdparty/liblj92`:
https://github.com/cgohlke/imagecodecs/tree/07b03834908db1ad41209bc175c495ff08cb9746/3rdparty/liblj92

MIT license; see LICENSE and the notices in the source files. Used to decode
lossless JPEG tiles in the UI and independently verify the Apertar encoder.
Recording uses Apertar's tile encoder, not the encoder in this library.

Local fix: handle a zero difference category without shifting by -1 in nextdiff.
