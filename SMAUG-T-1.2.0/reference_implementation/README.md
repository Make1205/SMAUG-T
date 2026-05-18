# SMAUG-T
SMAUG-T reference code (public release). Visit our [official website](https://www.kpqc.cryptolab.co.kr/smaug-t).

## Build

To build the SMAUG-T library and executables, there are the following prerequisites for KAT:

- [OpenSSL](https://www.openssl.org/)

Using cmake, you can build libraries and executables for each SMAUG-T mode.
The SMAUG-T modes are as follows: `smaugt_mode1`, `smaugt_mode3`, `smaugt_mode5`, `smaugt_modet`

\
Build in the SMAUG-T directory.

```bash
$ cd <path_to_SMAUG-T>
$ cmake --preset <debug|release>
$ cmake --build --preset <debug|release>
```

## Run

If the build was successful, result files will be generated in the `build/<<debug|release>>/bin` directory.
To run each test, execute as follows.

```bash
$ cd build/<debug|release>/bin
$ ./smaugt-mode1-main
$ ./smaugt-mode1-speed
$ ./smaugt-mode1-benchmark
$ ./smaugt-mode1-googletest
$ ./smaugt-modet-kat
```

## License

The codes and the specifications are under the MIT license.

## Acknowledgements

SMAUG-T is submitted to the Korean Post-Quantum Cryptography competition.
