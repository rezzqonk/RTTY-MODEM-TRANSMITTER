/*
Cara kompilasi: gcc fsk.c -o fsk -lasound -ldl -lm -lrt
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "soundtx.h"

#define RTIME 3000
FILE *frekam;
int16_t *sbuffer; //buffer integer 16 bit
long buffer_frames_48=RTIME*48000; //alokasi buffer dengan sampling 48k sesuai rpitx
long bitasound;
long offset=0;
int err;

int init_audio_tx()
{

  int i,j,k;
  int err;

  unsigned int rate = FS;// sampling rate
  int channel;
  unsigned int min,max;
  char namafile[30];
  double  pi= 3.141592653;

  devcID = malloc(sizeof(char) * 5);
  //   devcID = "plughw:1,0"; //default soundcard
   devcID = "plughw:2,0"; //default soundcard



  /********Membuka peranti dalam mode playback*************/

  if ((err = snd_pcm_open(&playback_handle, devcID, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
    fprintf(stderr, "tidak dapat membuka perangkat %s (%s)\n",devcID,snd_strerror(err));
    exit(1);
  } else {fprintf(stdout, "perangkat berhasil dibuka\n");}

  /**************Penambahan struktur parameter********************/

  if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
    fprintf(stderr, "mustahil untuk menambahkan parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "parameter tambahkan\n"); }
  if ((err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
    fprintf(stderr, "tidak dapat menginisialisasi parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "parameter diinisialisasi\n"); }


  /*******************Konfigurasi tipe akses ****************************/

  if ((err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
    fprintf(stderr, "tidak dapat mengonfigurasi jenis akses (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "jenis konfigurasi akses!\n"); }

  /********************************Format data****************************/

  if ((err = snd_pcm_hw_params_set_format(playback_handle, hw_params, format)) < 0) {
    fprintf(stderr, "tidak dapat mengatur format (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "format standar\n"); }

  /********************Mengatur frekuensi pengambilan sampel*************/

  //  if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0)) < 0) {

  if ((err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &rate, 0)) < 0) {
    fprintf(stderr, "tidak mungkin untuk mengatur frekuensi sampling (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "frekuensi sampling sesuai seting: %d (Hz)\n",rate); }

  /********************Setel jumlah saluran (Mono atau stereo)***************/

  err = snd_pcm_hw_params_get_channels_min(hw_params, &min);
  if (err < 0) {
    fprintf(stderr, "cannot get minimum channels count: %s\n",
	    snd_strerror(err));
    snd_pcm_close(playback_handle);
    return 1;
  }
  err = snd_pcm_hw_params_get_channels_max(hw_params, &max);
  if (err < 0) {
    fprintf(stderr, "cannot get maximum channels count: %s\n",
	    snd_strerror(err));
    snd_pcm_close(playback_handle);
    return 1;
  }

  if ((err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, min)) < 0) {
    fprintf(stderr, "tidak mungkin untuk mengatur jumlah saluran (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "jumlah saluran diset: %d ",min); }

  int tmp;
  snd_pcm_hw_params_get_channels(hw_params, &tmp);
  printf("(hasil: %i)\n", tmp);

  /////////////////////////////////////////////////////////////////////////

  if ((err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
    fprintf(stderr, "gagal untuk menerapkan parameter (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else {
    fprintf(stdout, "\nPenerapan parameter...\n");
  }

    snd_pcm_hw_params_free(hw_params);// membersihkan struktur parameter
    fprintf(stdout, "parameter dihapus\n");

  /************Persiapan sound card untuk playback***************************/

  if ((err = snd_pcm_prepare(playback_handle)) < 0) {
    fprintf(stderr, "gagal untuk menyiapkan kartu untuk registrasi (%s)\n",
	    snd_strerror(err));
    exit(1);
  }   else { fprintf(stdout, "siap playback... \n"); }


  err = snd_pcm_nonblock(playback_handle,0);  /* upaya mengatasi underrun saat ngirim per simbol */
  if (err < 0){
    printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
  }


  fprintf(stdout, "memori sudah dialokasikan!!\n");


}

//#define BSZ 256
#define BSZ 256
extern inline void put_audio_sample(float y)
{
  int err,nr;
  static short buf[BSZ];
  static int p=0;

  buf[p]=10000.*y;
  sbuffer[offset]=buf[p];
  p++;
  offset++;

  /* dibunyikan setiap 256 sampel*/
  if (p==BSZ) {
    p=0;
    if ((err = snd_pcm_writei(playback_handle, buf, BSZ)) == -EPIPE) {
      printf("\nXRUN...\n");
      snd_pcm_prepare (playback_handle);
    }else if (err < 0){
      printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(err));
    }
  }
}


float sintab[0x10000];
float *ampenv;
unsigned short freq0,freq1;
static unsigned short ph0,ph1;
int spb;

void send_bit(int bit)
{
        static int bits=0;
        int i;
        float c0,c1,out,amp;

        bits<<=1;
        bits|=(bit&1);
        bits&=3;

        for (i=0;i<spb;i++) {
            ph0+=freq0;
            ph1+=freq1;
            c0=sintab[ph0]; c1=sintab[ph1];
            switch(bits) {
            case 0:     amp=0.0; break;
            case 1:     amp=ampenv[i]; break;
            case 2:     amp=1.0-ampenv[i]; break;
            case 3:     amp=1.0; break;
            }
            out=c1*amp+(1.0-amp)*c0;
            put_audio_sample(out);
        }

}

void send_half_stop_bit()
{
        int i;
        for (i=0;i<spb/2;i++) {
            ph0+=freq0;
            ph1+=freq1;
            put_audio_sample(sintab[ph1]);
        }
}

enum {PAR_NONE=0, PAR_EVEN, PAR_ODD};

int nbits=8,nstop=1,par=PAR_NONE;

void send_byte(int data)
{
        int i,a,n1;

        send_bit(0);    //start bit
        a=data;
        for (i=n1=0;i<nbits;i++) {
            n1+=data&1;
            send_bit(data&1);
            data>>=1;
        }
        if (par) {
            if (par==PAR_EVEN) send_bit(n1&1);
            else send_bit((n1&1)^1);
        }
        for (i=0;i<nstop;i++) send_bit(1);    // Stop bits

}

//---------------- BAUDOT Translation ------------------
void send_baudot(unsigned char data)
{
	static int bdmode=-1;
	const char code[2][32]={  0,'E','\n','A',' ','S','I','U',
				'\r','D','R' ,'J','N','F','C','K',
				 'T','Z','L' ,'W','H','Y','P','Q',
				 'O','B','G' ,'^','M','X','V',0,
                                   0,'3','\n','-',' ',',','8','7',
	                        '\r','$','4' , 7 ,',','!',':','(',
				 '5','+',')' ,'2','#','6','0','1',
				 '9','7','&' ,'^','.','/','=',0};
	int i;

        //---- first time: send a escape for letters...
        if (bdmode<0) {
            send_byte(0x1f);
            send_half_stop_bit();
            bdmode=0;
        }
        if (data>='a' && data<='z') data-=32;

        // Search in the current half table
        for (i=0;i<32;i++) {
            if(code[bdmode][i]==data) {
                send_byte(i);
                send_half_stop_bit();
                return;
            }
        }
        // Search in the other half table
        for (i=0;i<32;i++) {
            if(code[bdmode^1][i]==data) {
                send_byte((bdmode)?0x1f:0x1b);
                send_half_stop_bit();
                send_byte(i);
                send_half_stop_bit();
                bdmode^=1;
                return;
            }
        }

}


//********* HDLC stuff *********
/* --------------------------------------------------------------------- */
/*
 * the CRC routines are stolen from WAMPES
 * by Dieter Deyke
 */

static const unsigned short crc_ccitt_table[] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};
/*---------------------------------------------------------------------------*/

static inline void append_crc_ccitt(unsigned char *buffer, int len)
{
 	unsigned int crc = 0xffff;

	for (;len>0;len--)
		crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ *buffer++) & 0xff];
	crc ^= 0xffff;
	*buffer++ = crc;
	*buffer++ = crc >> 8;
}

/*---------------------------------------------------------------------------*/

static inline int check_crc_ccitt(const unsigned char *buf, int cnt)
{
	unsigned int crc = 0xffff;

	for (; cnt > 0; cnt--)
		crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ *buf++) & 0xff];
	return (crc & 0xffff) == 0xf0b8;
}


void put_bit_NRZI(int bit)
{
	int i;
	static int oldst=0;

	if (!bit) oldst^=1;
	send_bit(oldst&1);
}

void put_idle(int n, int trama)
{
	int i,d;

	for (;n;n--) {
		d=trama;
		for (i=8;i;i--) {
			put_bit_NRZI(d&1);
			d>>=1;
		}
	}
}

void put_bit_hdlc(int bit)
{
	int i;
	static int n1=0;

	if (bit) n1++;	else n1=0;

	put_bit_NRZI(bit);

	if (n1==5) {	/* Insert 0 */
		put_bit_NRZI(0);
		n1=0;
	}
}

void put_hdlc(char *buf, int n)
{
	int i,j,d;

	append_crc_ccitt(buf,n);

	d=0x7e;		/* Start of packet Flag */

	for (i=0;i<7;i++) {
		put_bit_NRZI(d&1);
		d>>=1;
	}
	put_bit_hdlc(0);	/* reset Ones counter in put_bit_hdlc */

			/* packet's Data */
	for (i=n+2;i;i--) {
		d=*buf++;
		for(j=0;j<8;j++) {
			put_bit_hdlc(d&1);
			d>>=1;
		}
	}

	d=0x7e;		/* End of packet Flag */

	for (i=0;i<8;i++) {
		put_bit_NRZI(d&1);
		d>>=1;
	}
}

//------ Packet address stuffing
//------ Send an UI frame between two stations
void fill_address(unsigned char *buf,char *src,char *dst)
{
        int i;
        char *p;
        p=buf;
        for (i=0;i<6;i++) {if (!src[i]) break; *p++=src[i]<<1;}
        for (;i<6;i++) *p++=0x40;
        *p++=0xe0;
        for (i=0;i<6;i++) {if (!dst[i]) break; *p++=dst[i]<<1;}
        for (;i<6;i++) *p++=0x40;
        *p++=0x61; *p=0x03;
}

void caesar_cipher(char *text, int shift) {
  for (int i = 0; text[i] != '\0'; i++) {
    if (text[i] >= 'A' && text[i] <= 'Z') {
      text[i] = ((text[i] - 'A' + shift) % 26) + 'A';
    } else if (text[i] >= 'a' && text[i] <= 'z') {
      text[i] = ((text[i] - 'a' + shift) % 26) + 'a';
        }
    }
}

//----------------------------------------------------------------------


int main(int argc, char **argv)
{
	float f0,f1;
	int i,j,n,dr,hdlc=0,nr;
        unsigned char *p,buf[1024];
	FILE *fp;
	char c;

	if (argc<6) {
	    printf("\nUse     : %s f0 f1 speed format nama_file\n",argv[0]);
            printf("Examples:\n");
            printf("RTTY    : %s 1000 1170  45 8N2 %s.c\n",argv[0],argv[0]);
            printf("RTTY    : %s 1000 1170  45 5 %s.c\n",argv[0],argv[0]);
            printf("AX25    : %s 1000 1600 300 0H %s.c\n\n",argv[0],argv[0]);
            exit(0);
	}

	/*  Fbaca = fopen(argv[5],"rb");
        if (fbaca == NULL){
            printf("\nTidak ada file %s\n",argv[5]);
            exit(-1);
        }

        err = fseek(fbaca, 0, SEEK_END);
        off = ftell(fbaca);
        if (off == -1){
            printf("gagal ftell %s\n", argv[5]);
            exit(-1);
        }

        printf("panjang file %s adalah %ld\n", argv[5], off);
        rewind(fbaca);
        fclose(fbaca);
	*/

	FILE *file=fopen(argv[5],"r");
	if (!file){
	  perror("gagal membuka file");
	} 
	
	char text[1000]; // Buffer untuk membaca file
	char namafilecipher[80];
	printf("Masukkan nama file untuk menyimpan cipher: ");
	scanf("%s",namafilecipher);
     
	fp=fopen(namafilecipher,"wb");
	if (fp==NULL){
	  perror("gagal membuka file");}
	while (fgets(text, sizeof(text), file)) {
          caesar_cipher(text, 3); // Menggunakan Caesar cipher dengan shift 3
	  fprintf(fp,"%s",text);}
	fclose(file);
	fclose(fp);
	printf("cipher text berhasil disimpan dalam file: %s ",namafilecipher);

	fp=fopen(namafilecipher,"r");
	
        sbuffer=malloc(buffer_frames_48*2);

        f0=atof(argv[1]); f1=atof(argv[2]);
        dr=atoi(argv[3]);
        p=argv[4];
	argv[5]=namafilecipher;
	
        nbits=atoi(p);
        if (nbits>6) {
            switch(*++p) {
            case 'N':
            case 'n':       par=PAR_NONE; break;
            case 'E':
            case 'e':       par=PAR_EVEN; break;
            case 'o':
            case 'O':       par=PAR_ODD; break;
            case 'H':
            case 'h':       hdlc=1; break;
            }
            nstop=atoi(++p);
        }
        if(nbits==0) hdlc=1;
        if(!hdlc && nbits==5) {nstop=1; par=PAR_NONE;}

        freq0=f0*65536./FS; freq1=f1*65536./FS;
        spb=(FS+dr/2)/dr;

	/*jumlah bit data = off;
	  1 byte = 8 byte
	  jumlah sampel sinyal tone setiap baud = spb;

	 */

        printf("\nSettings\n------------------------------\n");
        printf("Space freq: %f Hz\n",(float)freq0*FS/65536.);
        printf("Mark  freq: %f Hz\n",(float)freq1*FS/65536.);
        printf("Data  rate: %f bps\n",(float)FS/spb);
        if (hdlc)
            printf("HDLC Packets\n");
        else {
          printf("Data  bits: %d\n",nbits);
          if (par) printf("Parity    : %s\n",(par==PAR_EVEN)?"Even":"Odd");
          printf("Stop  bits: %d\n",nstop);
        }
        printf("%d audio_samples/bit\n",spb);
        printf("------------------------------\n\n");

        if (*argv[5]=='-') fp=stdin;
        else if ((fp=fopen(argv[5],"r"))==NULL) {perror("fopen"); exit(1);}

        for (i=0;i<0x10000;i++) sintab[i]=cos(2.*M_PI*i/65536.);
        printf("> DDS table Init done\n");

        if ((ampenv=(float *)malloc(spb*sizeof(float)))==NULL) exit(1);
        for (i=0;i<spb-1;i++) ampenv[i]=0.5-0.5*cos(i*M_PI/(spb-1));
        ampenv[spb-1]=1.0;
        printf("> Ampl. Envelope table Init done\n");

	init_audio_tx();
        printf("> Audio Init (fs=%d Hz) done\n",FS);

        if (hdlc) {
            put_idle(25,0x7E);      /* Fill with flags */
	    //            fill_address(buf,"ASAL: Pusat","TUJUAN: Daerah");
            fill_address(buf,"Pusat: ","Daerah: ");
	    put_hdlc(buf,15);
	    put_idle(25,0x7E);
	    for(;;) {
		j=rand(); j&=63; j+=15;
		//                fill_address(buf,"ASAL: Pusat","TUJUAN: Daerah");
		fill_address(buf,"Pusat: Sukolilo","Daerah: Lamongan");
		p=&buf[50];
	    //		fill_address(buf,"Pusat:","Daerah:");
		//		p=&buf[15];
		for (i=15;i<j;i++) {
                        if(feof(fp)) break;
			*p++=getc(fp);
		}
		put_idle(25,0x7E);	/* Fill with flags */
		put_hdlc(buf,i);
                if (i<j) break;
	    }
        }else {
            printf("> Sending Space (BREAK) ..."); fflush(stdout);
            for (i=0;i<2.*dr;i++) send_bit(0);
            printf(" done\n");

            printf("> Sending Mark ..."); fflush(stdout);
            for (i=0;i<2.*dr;i++) send_bit(1);
            printf(" done\n");

            printf("> Sending data ...\n");

            i=0; j=spb/10; if (j==0) j=1;

	    //	    system("clear");
            while(!feof(fp)){
	      c=getc(fp);
	      //                if (nbits==5) send_baudot(getc(fp));
	      if (nbits==5){
		send_baudot(c);
		printf("%c",c);
	      }
	      else {
		printf("%c",c);
		//		send_byte(getc(fp));
		send_byte(c);
	      }
	      if (!(i%j)) //printf("\r%d bytes",i); fflush(stdout);
                i++;
	    }
        }
        printf("\n\n");
	sleep(30);   /* untuk menahan proses hingga produksi tone paripurna */
	snd_pcm_close(playback_handle);
	fclose(fp);


	char namafile[80], simpan [80];
	printf("Masukkan nama file untuk menyimpan tone(ex:tone.wav): ");
	scanf("%s",namafile);
	sprintf(simpan,"/home/pi/Desktop/rpitx/%s",namafile);
	frekam=fopen(simpan,"wb");
 	if (frekam == NULL){
        fprintf(stderr, "\nTidak ada file %s\n",simpan);
        exit(1);
	}

	short header[22];
	printf("besarnya data output tone: %ld byte\n",(offset*2)+44);
	for (i=0;i<22;i++)header[i]=0xAAAA;
	err=fwrite(header,2,22,frekam);
	err=fwrite(sbuffer,2,offset,frekam);
	free(sbuffer);
	fclose(frekam);
	sleep(10);
	
	}
