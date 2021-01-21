# DAQ for He-3 Counter with Analog Discovery 2 and Raspberry Pi

Author Keita Mizukoshi (Kobe Univ.)

 Jul. 11 2020
 Ver. 2.3


### Ver. 1からの修正点

  - DAQコード本体をPythonからC++ベースのものに変更
  - DAQコードに無信号状態とトリガーエラーの監視機能を追加
  - DAQコードに監視用に用いるテキストファイルの出力の追加
  - DAQのラッパーとしてのpython run スクリプトの追加
  - データをROOTファイルに変換するROOT スクリプトの追加


## 概要

He-3比例計数管を用いた地下での環境中性子測定のためのデータ収集系を制作, 試験した.

データ収集においては, 小型で安定して動作すること, 低 sampling rateで長い波形を取得できることが求められる. 一方で, 地下環境では中性子のレートは非常に低いため, データ取得レート性能は放射線源をもちいた較正に耐えうる程度でよい. そのため2万円強で購入できるUSBオシロスコープ Analog Discovery 2 とその制御用のRaspberry pi 4でDAQを制作し, テストパルスを入れて動作を試験した.

Analog Discovery 2はUSBでraspberry pi4に接続し, 通信, 給電をおこなう. 今回の環境では問題にならなかったが, 電力供給が不足する場合にはAnalog Discovery 2にDCで電源供給することも可能である.

試験に用いたソフトウェアは `/home/pi/ad2_daq` 以下を参照されたい. 特に`bin/daq.cpp` がAnalog Discovery 2と通信し波形取得するスクリプトである. その他のスクリプトについては後述する.

## Prerequirement

  - Adept 2 runtime
  - WaveForms

  `dpkg -i`などでインストールする.

## build
```
$ cd ad2_daq
$ mkdir build
$ cd build
$ cmake ../sources
$ make install
$ ~/ad2_daq/bin/daq ## Show Usage
```

## `sources/daq.cpp`

本コードはAnalog Discovery 2と通信し, トリガーを超えた信号についてデータを取得, ASCIIファイルとして保存する. コマンドラインから引数で制御可能なので, 長期ランを行う際にはこのバイナリを連続して実行する仕組みを作ればよい. オプションの内容は引数を空にして実行することで確認可能である.

```
pi@raspberrypi:~/ad2_daq/build $ ~/ad2_daq/bin/daq
Usage:
./daq Frequency(Hz) Trigger_level(V) rise/fall Trigger_position(s) Entries File_name_prefix
i.e., ./daq 20000000.0 -1.0 rise 0.0 100 test1

```
`File_name_prefix`はPathも含む.

実行すると `File_name_prefix.txt`というオプションの設定情報や実行状況を書き出すファイルと,波形データを含むファイル `File_name_prefix.xxxx.dat` (1000イベントごとにxxxxがインクリメント)が生成される.

このバイナリは`bin/run.py`を通して扱うと便利である. スクリプトの最初の十数行の変更で,継続してデータを取集し続ける.
sub_dir名を変更しなくても,自動で新しいファイルを作成する.
Runにはこのスクリプトを用いて, 長期Runには`nohup`などをつかえばよい.
```
def run():

    ## Edit HERE ##

    data_dir = '/home/pi/ad2_daq/data/' # with the last slash
    sub_dir = '20200705'
    entries = 10000

    frequency = 10000000.0 # Hz For PMT test
    #frequency = 20000.0 # Hz For He-3
    trigger_level = -1.0 # V
    trigger_type = 'rise' # rise or fall
    trigger_position = 3.e-4 # s

    daq_cmd = '/home/pi/ad2_daq/bin/daq'

    ###############
```

実行例

```
pi@raspberrypi:~/ad2_daq $ ~/ad2_daq/bin/run.py
Manual copy: /home/pi/ad2_daq/bin/autocopy.sh /home/pi/ad2_daq/data/20200705
/home/pi/ad2_daq/bin/daq 10000000.0 -1.0 rise 0.0003 10000 /home/pi/ad2_daq/data/20200705/sub0000
Open automatically the first available device
Starting repeated acquisitions.
Ev. 0
...
```

設定情報ファイル

```
pi@raspberrypi:~/ad2_daq $ tail -f data/20200705/sub0000.txt
File name prefix       : /home/pi/ad2_daq/data/20200705/sub0000
Sampling Frequency (Hz): 1e+06
Trigger Level (V)      : 0.1
Trigger Type           : Falling Negative
Trigger Position (s)   : 0.003
Range (V)              : 5
Offset (V)             : 0
Number of Events       : 10000
Rate(Hz) 10 1594135974 2.5
Rate(Hz) 20 1594135977 3.33333
Rate(Hz) 30 1594135980 3.33333
...

```
設定情報のあとは,モニター用に10イベント毎に情報を更新する. `Rate(Hz) EventNo Timestamp Rate`のフォーマットで続き, 最終行には終了ステータスとメッセージが出力される. 終了ステータスにかかわらず,次のSubrunは自動的に開始される.
コピースクリプトとは別に,この設定情報を読みだすだけで,最低限の監視が可能である.


波形ファイル

波形ファイルの出力はイベントごとに,イベント番号と大まかなTrigger timingのタイムスタンプ(Raspberry Pi)を並べたheaderとそれに続く8192行の波高値 (V), ファイルを書き込み終わった時間のタイムスタンプのfooterで構成されている. 以下に例を示す. これらのタイムスタンプから大まかなlivetimeが計算できる.

```
#. 0 1594135970 258551
-48
-24
-20
-32
...
36
20
24
28
1594135970 262922
#. 1 1594135970 482759
16
24
12
20
```
ADC値は`[ADC]*[Range(V)]/65536+[Offset(V)]`で電圧値に直すことができる.

## 終了
特に考えずに作ったので, pythonのラッパースクリプトと実行ファイルの`daq`を両方killする. Ctl-Cでも可.

## こまごまとしたスクリプトなど
`autocopy.sh` はrun.pyから呼ばれるスクリプトでデータを逐次コピーする. コピー中は`bin/lock`が作られ, 多重起動しないようになっている. `safe_rm.py`は自動実行されない.必要があればcronなどで定期的に実行すればDisk圧迫を避けることができる.

