# R909-DSP
R909-DSP is a receiver for the airband and the FM broadcasting. 
It is configured with Si5351a, Si4732, and TA2003 mainly.
TA2003 and Si5351a are sharing a first mixer and a local oscillator.
Si4732 is acting as a mother 21.4MHz receiver for the airband and the FM radio itself.
Based on PU2CLR's advice, I adopted PU2CLR Si4735/32 library for this sketch.
There are 2 kinds of display versions, as the one for OLED and the other for WE1602A. 

Operation is depending on the rotary encoder and the rotary encoder's push switch.
There are 3 major modes, FUNC of the functions selecting, parameter selection at every function mode, and to memorize the parameter into EEPROM.
FUNC mode is to select every function. You shall select the function by rotating and determine the one by pressing once.
In every function mode you can change the parameters by rotating and store the parameters on to EEPROM bt pressing twice.
To escape from every function mode, to press the rortary encoder switch only once.
Rotating, once pressing, and twice pressing is the identical operation for this sketch.

There are two kinds of PCBs as Panel and RF. They are connecting with pinhedder connector. 
The ATmega328P with the Arduino boot loader, the switches, and the display are locating on the Panel PCB.

I resumed the uploaded files list in "r909-DSP_list_1.jpg" and "r909-DSP_list_1.jpg".

I introduced how to operate on YOUTUBE as below:
1602A debugging "https://www.youtube.com/watch?v=wb-YdMU-vz0"
OLED panel "https://www.youtube.com/watch?v=UpdDh7BYJYI" "https://www.youtube.com/shorts/03Hpvuk5BYI"

If you use my Gerber files or information to make PCB, please resister PCBGOGO via "https://www.pcbgogo.jp/promo/nobcha23"

I'm running BLOG and please visit "https://nobcha23.hatenadiary.com/"

I'm much thanking Mr.Jason,9H5BM for giving me the idea of this project, PU2CLR and JF3HZB for showing me the sketch examples, Cesar Sound for introducing me the display layout example, and many people for giving me much information.

     Jason kits, 9H5BM　https://www.facebook.com/profile.php?id=100012257914763
     Ricardo Lima Caratti, pu2clr　 https://github.com/pu2clr
     上保 徹志 (Tetsuji Uebo), JF3HZB　 https://tj-lab.org/2017/03/13/si5351/
     CesarSound https://www.hackster.io/CesarSound/10khz-to-225mhz-vfo-rf-generator-with-si5351-version-2-bfa619

After then I found the cause of low sensitivity because of error pin assignment for 2SC3355 as ECB. BEC is correct so please change insertion of pins.
And also Xule-san sent me the comnents. I reviced the errata list. Please refer the coils_R909-SDR.pdf for the front end assembling.
I'm still debugging and revicing the sketch.

 nobcha　　　E-MAIL:”nobcha48 at gmail.com"

![R909-DSP1_pcb](https://github.com/user-attachments/assets/a07b804d-5c57-4b06-bd32-474270569fa5)

D909-DSPというのはSi4732を使った、航空無線受信とFM放送受信ができる自作ラジオです。
Si4732を21.4MHｚ親受信部に使い、ヘテロダインミキサーはTA2003にSi4751a局発モジュールを使います。
FM放送受信はSi4732で行います。Si4732の制御はPU2CLRのライブラリを使い、Arduinoで行います。
表示はOLEDを用い、CesarSoundさんのVFO表示を参考にしました。1602A版もあります。
操作はロータリーエンコーダとロータリーエンコーダプッシュスイッチで行います。
50チャンネルのメモリーを登録し、自動スキャンなどもできます。

日々の実験進捗などは次のブログにありますので、ご参考に願います。"https://nobcha23.hatenadiary.com/"

基板構成はRF部とPanel部で88ｘ38ｘ100のアルミケースに入れるようにしました。
ＹＯＵＴＵＢＥを見ていただけるとわかりますが、動作安定で音声もクリアです。
--感度が悪い原因は2SC3355のPCBパターン間違い（ECB)でした。正しくはBECなので、TRを反対向きにして、EとCを交差させ実装願います。


基板はＰＣＢＧＯＧＯの協力を得ました。提供するガーバーファイルはそのためＰＣＢＧＯＧＯ向けになっています。
次のエントリーから登録してお使いください。"https://www.pcbgogo.jp/promo/nobcha"

関連データ類をアップロードしましたが、名前と用途の説明リストも用意しました。
R909_20240408JA.docx

また、このプロジェクトに際し、色々な方の情報を参考にしました。感謝します。
     Jason kits, 9H5BM　https://www.facebook.com/profile.php?id=100012257914763
     Ricardo Lima Caratti, pu2clr　 https://github.com/pu2clr
     上保 徹志 (Tetsuji Uebo), JF3HZB　 https://tj-lab.org/2017/03/13/si5351/
     CesarSound https://www.hackster.io/CesarSound/10khz-to-225mhz-vfo-rf-generator-with-si5351-version-2-bfa619

IF段の2SC3355部がおかしいことに気が付きました。TRのピン配置が間違っていました。この問題が解消し、感度は-100ｄBmより良くなり、実用的になってきました。
またシュールさんからご指摘をいただき、部品名など情報追加で、errata240624.jpgをアップデータとしました。
まだ、いくつか心配点があり、デバッグ進行中です。

 nobcha  E-MAIL:”nobcha48 at gmail.com"
