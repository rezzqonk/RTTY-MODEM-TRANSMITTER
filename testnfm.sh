#!/bin/sh

#This is only a Narraw Band FM modulator, for FM broadcast modulation , use PiFMRDS
# Need to use a direct FM modulation with librpitx and not using IQ : TODO
echo "If you need to test broadcast FM, use PiFMRDS"
#(while true; do cat sampleaudio.wav; done) | csdr convert_i16_f | csdr gain_ff 2500 | sudo ./sendiq -i /dev/stdin -s 24000 -f 434e6 -t float 1
#(while true; do cat simbol48_new.wav; done) | csdr convert_i16_f \
#(while true; do cat sampleaudio.wav; done) | csdr convert_i16_f \
#(while true; do cat ss-4.wav; done) | csdr convert_i16_f \
#hdlc_v2.wav

#(cat waributone.wav) | csdr convert_i16_f \
 # (cat namrattone.wav) | csdr convert_i16_f\
    #(cat tigartone.wav) | csdr convert_i16_f \
    #(cat waluhtone.wav) | csdr convert_i16_f \
    #(cat satutone.wav) | csdr convert_i16_f \
 (cat gambarrtty2.wav) | csdr convert_i16_f \
  | csdr gain_ff 7000 | csdr convert_f_samplerf 20833 \
  | sudo ./rpitx -i- -m RF -f "$1"

