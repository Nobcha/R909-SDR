# R909-DSP
R909-DSP is a radio for the airband and the FM broadcasting. 
It is configured with Si5351a, Si4732, and TA2003 mainly.
TA2003 and Si5351a are sharing the first mixer and the local oscillator.
Si4732 is acting as a mother 21.4MHz receiver for the airband and the FM radio itself.
According with PU2CLR's advice I adopted PU2CLR Si4735/32 library for this sketch.
There are 2 kinds of display version, as the one for SSD and the other for WE1602A. 

Operation is depending on the rotary encoder and the rotary encoder's push switch.
There are 3 major modes, as FUNC, parameter selection at every function mode, and to memory the parameter into EEPROM.
FUNC mode is to select every function. You shall select the function by rotating and determine the one by single pushing.
In every function mode you can change the parameter by rotating and store the parameter on to EEPROM bt double pushing.
To escape out from every function mode to push rortary encoder switch single.
Rotating, one pushing, and two pushing is the identical operation for this sketch.

There are two kinds of PCBs as Panel and RF to compose the radio. They are connecting by pinhedder connector. 
The ATmega328P with the Arduino boot loader, the switches, and the display are locating on the Panel PCB.

I resumed the uploaded files list as "r909-DSP_list_1.jpg" and "r909-DSP_list_1.jpg".

I introduced how to operate on YOUTUBE as below:
1602A debugging "https://www.youtube.com/watch?v=wb-YdMU-vz0"
OLED panel "https://www.youtube.com/watch?v=UpdDh7BYJYI" "https://www.youtube.com/shorts/03Hpvuk5BYI"

If you use my Gerber files or information to make PCB, please resister PCBGOGO via "https://www.pcbgogo.jp/promo/nobcha23"

I'm running BLOG and please visit "https://nobcha23.hatenadiary.com/"

I'm much thanking Mr.Jason,9H5BM to give me the idea of this project, PU2CLR and JF3HZB to show me the sketch examples, Cesar Sound to introduce me the display layout example, and many people to give me many information.

     Jason kits, 9H5BM　https://www.facebook.com/profile.php?id=100012257914763
     Ricardo Lima Caratti, pu2clr　 https://github.com/pu2clr
     上保 徹志 (Tetsuji Uebo), JF3HZB　 https://tj-lab.org/2017/03/13/si5351/
     CesarSound https://www.hackster.io/CesarSound/10khz-to-225mhz-vfo-rf-generator-with-si5351-version-2-bfa619

 nobcha　　　E-MAIL:”nobcha48 at gmail.com"

D909-DSPというのはSi4732を使った、航空無線受信とFM放送受信ができる自作ラジオです。
Si4732を21.4MHｚ親受信部に使い、ヘテロダインミキサーはTA2003にSi4751a局発モジュールを使います。
FM放送受信はSi4732で行います。Si4732の制御はPU2CLRのライブラリを使い、Arduinoで行います。
表示はOLEDを用い、CesarSoundさんのVFO表示を参考にしました。
操作はロータリーエンコーダとロータリーエンコーダのプッシュスイッチで行います。
50チャンネルのメモリーを登録し、自動スキャンなどもできます。

日々の実験進捗などは次のブログにありますので、ご参考に願います。"https://nobcha23.hatenadiary.com/"

基板構成はRF部とPanel部で88ｘ38ｘ100のアルミケースに入れるようにしました。
ＹＯＵＴＵＢＥを見ていただけるとわかりますが、動作安定で音声もクリア、難点はいまいちの感度ぐらいです。

基板はＰＣＢＧＯＧＯの協力を得ました。提供するガーバーファイルはそのためＰＣＢＧＯＧＯ向けになっています。
次のエントリーから登録してお使いください。"https://www.pcbgogo.jp/promo/nobcha23"

また、このプロジェクトに際し、色々な方の情報を参考にしました。感謝します。
     Jason kits, 9H5BM　https://www.facebook.com/profile.php?id=100012257914763
     Ricardo Lima Caratti, pu2clr　 https://github.com/pu2clr
     上保 徹志 (Tetsuji Uebo), JF3HZB　 https://tj-lab.org/2017/03/13/si5351/
     CesarSound https://www.hackster.io/CesarSound/10khz-to-225mhz-vfo-rf-generator-with-si5351-version-2-bfa619

 nobcha  E-MAIL:”nobcha48 at gmail.com"
