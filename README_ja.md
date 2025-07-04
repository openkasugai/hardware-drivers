# hardware-drivers (for Experimental Branch)

> [!WARNING]  
> 本ソフトウェアは`hardware-design`リポジトリの同名ブランチのものと組み合わせて使用する必要があります。（mainブランチのものと使用することはできません）

## はじめに

`hardware-drivers`は、experimentalブランチのOpenKasugai Hardwareを制御するためのライブラリ、ドライバ、サンプルプログラムです。

**このソフトウェアはexperimentalブランチ専用です。**

## ディレクトリ構成

```
hardware-drivers :
       -+- include     : C header files
        +- src         : C++ source files
        +- test        : test codes
        +- lib         : generate for libraries after build.
```

## ビルド手順

### 準備

- `hardware-design`リポジトリを参照して、事前にBitstreamの生成と書き込みを行ってください

- cmakeのインストール
  - googletestのビルド時に必要です
  - Vitis環境へのパスを通していない環境での実行が必要です（必要なライブラリが変更されてしまうため）

### ドライバとライブラリのビルド

- リポジトリのトップディレクトリでビルドを実行します。
  ```
  $ make src 
  ```

### ドライバのインストール

- テストコードのビルド前に、ドライバのインストールを実行します。
  ```
  $ make install-drivers
  ```
  - 'xse?\_'という名前で始まるデバイスファイルが作成されます。
  - FPGAデザインにaxiliteポートを使用する機能がある場合は、対応するデバイスファイルが作成されているか確認してください。

### テストコードのビルド

- リポジトリのトップディレクトリでビルドを実行します。
  ```
  $ make test
  ```

## ライセンス

|項目|パス|ライセンス|
|:--|:--|:--|
|テストコード|`/test/`|BSD 3-Clause License|
|ツール|`/src/tools/`|BSD 3-Clause License|
|ライブラリ|`/src/lib/`|BSD 3-Clause License|
|ドライバ|`/src/drivers/`|GNU General Public License v2.0|