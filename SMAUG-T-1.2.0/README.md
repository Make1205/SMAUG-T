# SMAUG-T

Reference source code for SMAUG-T -- efficient and compact KpqC-winning, lattice-based, post-quantum Key Encapsulation Mechanism (KEM). Visit our [official website](https://www.kpqc.cryptolab.co.kr/smaug-t) for more information.

SMAUG-T is an efficient post-quantum key encapsulation mechanism (KEM) whose security is based on the hardness of the lattice problems, Module-Learning-with-Errors (MLWE) and Module Learning-with-Roundings (MLWR). SMAUG-T enjoys a conservative secret key security relying on the MLWE problem and an efficient ephemeral key generation relying its security on the MLWR problem. SMAUG-T follows the recent approaches in designing the post-quantum-secure KEMs in the Quantum Random Oracle Model (QROM) while maintaining efficiency. An additional parameter set, TiMER (Tiny sMaug using Error Reconciliation), is a newly proposed targetting security level 1, which exploits D2 encoding for lower decryption failure probability (DFP).

## Build

Please refer to [Reference Implementation README](reference_implementation/README.md).

## License

The codes and the specifications are under the MIT license.

## Contributors

Team SMAUG-T
- Jung Hee Cheon (Seoul National University (SNU) & CryptoLab Inc.)
- Hyeongmin Choe (University of Luxembourg)
- Joongeun Choi (Defense Counter-intelligence Command (DCC))
- Dongyeon Hong (Samsung Electronics)
- Jeongdae Hong (CryptoLab Inc.)
- Chi-Gon Jung (DCC)
- Honggoo Kang (DCC)
- Minhyeok Kang (CryptoLab Inc.)
- Taekyung Kim (CryptoLab Inc.)
- Jeongbeen Ko
- Janghyun Lee (DCC)
- Seonghyuck Lim (DCC)
- Aesun Park (DCC)
- Seunghwan Park (DCC)
- Jungjoo Seo (CryptoLab Inc.)
- Hyoeun Seong
- Junbum Shin (CryptoLab Inc.)
- MineJune Yi
