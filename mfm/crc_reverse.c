// This shows various stuff for reverse engineering CRC polynomials

// Standard header files
#define DEBUG 0
#define PRINT_ERR 0
#define WINDOW_FILTER 0
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "inc/crc_ecc.h"

#define ARRAYSIZE(x)  (sizeof(x) / sizeof(x[0]))

// Most significant bit first (big-endian)
// x^16+x^12+x^5+1 = (1) 0001 0000 0010 0001 = 0x1021
int calc_crc16(int byte, int crc, int poly)
{
   int j;
   crc = crc ^ (byte << 8);
   for (j = 1; j <= 8; j++) {   // Assuming 8 bits per byte
      if (crc & 0x8000) {       // if leftmost (most significant) bit is set
         crc = (crc << 1) ^ poly;
      } else {
         crc = crc << 1;
      }
   }
   return crc & 0xffff;
}

int calc_crc32(int byte, int crc, int poly)
{

   int j;
   crc = crc ^ (byte << 24);
   for (j = 1; j <= 8; j++) {   // Assuming 8 bits per byte
      if (crc & 0x80000000) {   // if leftmost (most significant) bit is set
         crc = (crc << 1) ^ poly;
      } else {
         crc = crc << 1;
      }
   }
   return crc;
}
unsigned int calc_crc32rb(int byte, unsigned int crc, unsigned int poly)
{
   int j;
   for (j = 1; j <= 8; j++) {   // Assuming 8 bits per byte
      if (crc & 0x1) {   // if rightmost (least significant) bit is set
         crc = ((crc ^ poly) >> 1) | 0x80000000;
      } else {
         crc = (crc >> 1);
      }
   }
   crc = crc ^ (byte << 24);
   return crc;
}


int calc_crc32r(int byte, int crc, int poly)
{
   int j;
   crc = crc ^ (crc_revbits(byte, 32) & 0xff000000);
   for (j = 1; j <= 8; j++) {   // Assuming 8 bits per byte
      if ((crc & 0x80000000)) { // if leftmost (most significant) bit is set
         crc = (crc << 1) ^ ((crc_revbits(poly, 32) << 1) | 1);
      } else {
         crc = crc << 1;
      }
   }
   return crc;
}

void ecc() {
   uint64_t crc;
   uint8_t bytes[512+4];
   int i;
   CRC_INFO crc_info;
   int span;
   int crc_mask, crc32, poly;

   //poly = 0x140a0445;
   crc_info.poly = 0x00a00805;
   crc_info.length = 32;
   crc_info.init_value = ((uint64_t) 1 << crc_info.length) - 1;
   crc_info.ecc_max_span = 11;

   memset(bytes, 8, sizeof(bytes));

   crc = crc64(bytes, sizeof(bytes)-4, &crc_info);
   i = sizeof(bytes)-4;
   bytes[i++] = (crc >> 24);
   crc <<= 8;
   bytes[i++] = (crc >> 24);
   crc <<= 8;
   bytes[i++] = (crc >> 24);
   crc <<= 8;
   bytes[i++] = (crc >> 24);
   crc <<= 8;

   crc = crc64(bytes, sizeof(bytes), &crc_info);
   printf("Check crc %08x\n",crc);
   //bytes[9] = 0x09;
   //bytes[10] = 0x00;
   bytes[14] = 0xd0;
   bytes[15] = 0x08;
   crc = crc64(bytes, sizeof(bytes), &crc_info);
   printf("err crc32 %08x\n",crc);
   span = ecc64(bytes, sizeof(bytes), crc, &crc_info);
   printf("ECC span %d\n", span);
   crc = crc64(bytes, sizeof(bytes), &crc_info);
   printf("Fixed crc32 %08x\n",crc);
#if 0
   poly = 0x00a00805;
   crc = crc_info.init_value;
   for (i = 0; i < sizeof(bytes); i++) {
      crc = crc64(bytes[i], crc, &crc_info);
   }
   printf("err crc32 %08x\n",crc);
   crc_mask = ((1 << 10) - 1) << (32 - 10);
   printf("mask %08x\n",crc_mask);
   crc32 = crc_revbits(crc, 32);
   for (i = 0; i < sizeof(bytes); i++) {
      crc32 = calc_crc32(0, crc32, ((crc_revbits(poly, 32) << 1) | 1));
      if ((crc32 & crc_mask) == 0) {
         unsigned int fix;
         fix = crc_revbits(crc32, 32) >> 16;
         printf("Fix %04x\n",fix);
         printf("Fix bytes at %d %02x %02x\n", i, bytes[sizeof(bytes)-i-1] ^ (fix >> 8),
           bytes[sizeof(bytes)-i] ^ (fix & 0xff));
      }
   }
#endif
}
main()
{
   unsigned int crc32 = 0, crc32b;;
   uint64_t crc64o;
   uint64_t poly64;
   CRC_INFO crc_info;
   int i,j;
   int poly, init;
   crc32b = 0;
   struct timeval tv_start;
   double start, end;
//0x00,0x00,0x82,0x20,0x00,0x71,0x06
uint8_t bytes[] = {
0xa1,0xfe,0x00,0x00,0x03,0x06,0xcc,0x1b
};
   int tot_crc = 0;

   find_poly();
//exit(0);
#if 0
   poly = 0x00a00805;
   gettimeofday(&tv_start, NULL);
   start = tv_start.tv_sec + tv_start.tv_usec / 1e6;
   for (i = 0; i < 100000; i++) {
     crc32 = 0xffffffff;
     for (j = 0; j < sizeof(bytes); j++) {
         crc32 = calc_crc32(bytes[j], crc32, poly);
     }
     tot_crc += crc32;
   }
   gettimeofday(&tv_start, NULL);
   end = tv_start.tv_sec + tv_start.tv_usec / 1e6;
   printf("Crc32 time %.2f, %.2f/sec tot %x\n",end-start, i/(end-start), tot_crc);

#endif
   tot_crc = 0;
for (crc_info.poly = 0; crc_info.poly <= 0xffff; crc_info.poly++) {
   crc_info.length = 16;
   crc_info.init_value = 0;

   gettimeofday(&tv_start, NULL);
   start = tv_start.tv_sec + tv_start.tv_usec / 1e6;
     crc64o = crc64(bytes, sizeof(bytes), &crc_info);
     crc64o = crc64o ^ 0xff;
//printf("crc %d %llx\n",sizeof(bytes), crc64o);
     if (crc64o == 0x758a) 
        printf("Poly %llx\n",crc_info.poly);
     tot_crc += crc64o;
   gettimeofday(&tv_start, NULL);
   end = tv_start.tv_sec + tv_start.tv_usec / 1e6;
}
//exit(1);
     
   //ecc();
  // return;
  
   // This shows finding the CRC polynomial by modifying bytes. If the
   // CRC is in the header then the changing sector number field will give
   // you the data for this. If you can write data sectors or are lucky
   // you can get the proper pattern there also. All the data must be the
   // same other than the one byte.
   // See http://www.cosc.canterbury.ac.nz/greg.ewing/essays/CRC-Reverse-Engineering.html
   // Find a field that is 0 1, 2, 4, 8 etc. xor the CRC for the 0 with the
   // 1, 2, 4 ... CRC. Then take example (CRC of 4) ^ (CRC of 2 >> 1) This 
   // should be 0 or the CRC polynomial. This assumes MSB first CRC order.
   poly = 0x00a00805;
   crc32b = 0;
   for (i = 0; i < 8; i++) {
      int crc_hld[2], j;
      for (j = 0; j < 2; j++) {
         crc32 = calc_crc32(0, 0xffffffff, poly);
         crc32 = calc_crc32(1, crc32, poly);
         crc32 = calc_crc32(2, crc32, poly);
         if (j == 1) {
            crc32 = calc_crc32(0, crc32, poly);
         } else {
            crc32 = calc_crc32(1 << i, crc32, poly);
         }
        // crc32 = calc_crc32(3, crc32, poly);
       //  crc32 = calc_crc32(4, crc32, poly);
         crc_hld[j] = crc32; // Get CRC for data
      }
      // This removes the effect of the CRC initialization value and
      // and xor/invert of the final CRC value
      crc32 = crc_hld[0] ^ crc_hld[1];  
      // Now this print should be either the polynomial or zero
      printf("%d %08x %08x\n", 1 << i, crc32, crc32 ^ (crc32b << 1));
      crc32b = crc32;
   }
#if 0
{uint8_t buf[16];
 static uint8_t buf2[1164];
uint8_t buf3[] = {
#include "/tmp/t"
};
 int loc = 10;
 uint32_t last_crc = 0;
 uint32_t first = 0;
   
   memset(buf,0,sizeof(buf));
   buf[15] = 0x12;
      buf[loc] =  0x3;

   //crc_info.poly = 0x00a00805;
   crc_info.poly = 0x140a0445;
   //crc_info.poly = 0x0104c981;
   //crc_info.poly = 0x41044185;

   crc_info.length = 32;
   crc_info.init_value = 0xffffffff;
   crc_info.ecc_max_span = 0;

   crc32 = crc64(buf2, sizeof(buf2), &crc_info);
   printf("1164 crc %x\n",crc32);

   crc32 = crc64(buf3, sizeof(buf3), &crc_info);
   printf("buf3 crc %x\n",crc32);

   crc32 = crc64(buf, 16, &crc_info);
   first = crc32;
   printf("buf %x %x\n",buf[loc], crc32);
   for (i = 0; i < 8; i++) {
      buf[loc] = (1 << i) | 0x3;
      crc32 = crc64(buf, 16, &crc_info);
      crc32 = crc32 ^ first;
      printf("buf %x %x\n",buf[loc], crc32);
      printf("crc %x\n",(last_crc << 1) ^ crc32);
      last_crc = crc32;
   }
return;
}
#endif

   // This shows using the reverse polynomial to find the first 4 bytes
   // of the CRC value.
   init = 0xffffffff;
   crc32 = calc_crc32(0x10, init, poly);
   crc32 = calc_crc32(0x20, crc32, poly);
   crc32 = calc_crc32(0x30, crc32, poly);
   crc32 = calc_crc32(0x40, crc32, poly);
   printf("crc %08x %08x %08x\n", crc32, (int) ~crc_revbits(crc32, 32), 
       (int) crc_revbits(crc32, 32));
   crc32 = calc_crc32(0x80, crc32, poly);
   printf("crc %x rev %x\n", crc32, crc_revbits(crc32, 32));
   init = 0;
   crc32b = crc32;
   crc32 = calc_crc32r(crc32b >> 0, init, poly);
   crc32 = calc_crc32r(crc32b >> 8, crc32, poly);
   crc32 = calc_crc32r(crc32b >> 16, crc32, poly);
   crc32 = calc_crc32r(crc32b >> 24, crc32, poly);
   crc32 = calc_crc32r(0x80, crc32, poly);
   // If init is all ones we need to invert the final result.
   // if init 0 no invert needed. The revbits is still needed.
   printf("crc %08x %08x %08x\n", crc32, (int) ~crc_revbits(crc32, 32), 
       (int) crc_revbits(crc32, 32));

   // This doesn't work. Seems like calculating backwards should be doeable
   init = crc32b;
   crc32 = calc_crc32rb(0x80, init, poly);
   printf("crcrb %08x %08x %08x\n", crc32, (int) ~crc_revbits(crc32, 32), 
       (int) crc_revbits(crc32, 32));

   crc32 = 0;
   poly = 0x104c981;
   for (i = sizeof(bytes)-1; i >= 0; i--) {
      crc32 = calc_crc32r(bytes[i], crc32, poly);
      printf("%d crc %08x %08x %08x\n", i, crc32, (int) ~crc_revbits(crc32, 32), 
       (int) crc_revbits(crc32, 32));
   }

   crc32 = 0;
   poly = 0x140a0445;
   for (i = sizeof(bytes)-1; i >= 0; i--) {
      crc32 = calc_crc32rb(bytes[i], crc32, poly);
      printf("crc %d %08x\n",i, crc32);
   }
return;
   poly = 0x1021;
   // This was for finding which bytes they started the CRC with.
   // I inverted the CRC bytes assuming the init all ones above.
   uint8_t data[] = { 0, 0, 0xa1, 0xfe, 0xfe, 0, 0, 0x0, 0, 0x84 ^ 0x00, 0xc0 ^ 0x00 };
   uint8_t data3[] = { 0, 0, 0xa1, 0xfe, 0xfe, 0, 0, 0x0, 5, 0xd4 ^ 0x00, 0x65 ^ 0x00 };
   uint8_t data4[] = { 0, 0, 0xa1, 0xfe, 0xfe, 0, 0, 0x0, 6, 0xe4 ^ 0x00, 0x06 ^ 0x00 };
int crc32_init, xorout, match;
#if 0
   crc32 = 0x1234;
   for (i = 2; i < ARRAYSIZE(data)-2; i++) {
      crc32 = calc_crc32(data[i], crc32, poly);
   }
   printf("CRC %x\n",crc32);
   crc32 = (crc32 & 0xffff) ^ 0x5678;
   data[9] = crc32 >> 8;
   data[10] = crc32;
   printf("CRCx %x\n",crc32);
   crc32 = 0x1234;
   for (i = 2; i < ARRAYSIZE(data3)-2; i++) {
      crc32 = calc_crc32(data3[i], crc32, poly);
   }
   printf("CRC %x\n",crc32);
   crc32 = (crc32 & 0xffff) ^ 0x5678;
   data3[9] = crc32 >> 8;
   data3[10] = crc32;
   printf("CRCx %x\n",crc32);
#endif
for (crc32_init = 0; crc32_init < 0x10000; crc32_init++) 
//crc32_init = 0;
{
//crc32_init = 0x1234;
   crc32 = crc32_init;
   for (i = 2; i < ARRAYSIZE(data)-2; i++) {
      crc32 = calc_crc16(data[i], crc32, poly);
   }
   xorout = 0x84c0 ^ (crc32 & 0xffff);
   //xorout = 0x4d6e ^ (crc32 & 0xffff);
   //xorout = 0x0 ^ (crc32 & 0xffff);
//printf("xorout %x %x\n",xorout, crc32);
   crc32 = crc32_init;
   for (i = 2; i < ARRAYSIZE(data3)-2; i++) {
      crc32 = calc_crc16(data3[i], crc32, poly);
   }
   //xorout = 0x0 ^ (crc32 & 0xffff);
//printf("xorout %x %x\n",xorout, crc32);
   match = 0;
   if (((crc32 ^ xorout) & 0xffff) == 0xd465) {
      //printf("Match init %x %x\n",crc32_init, xorout);
      match = 1;
   }
   crc32 = crc32_init;
   for (i = 2; i < ARRAYSIZE(data4)-2; i++) {
      crc32 = calc_crc16(data4[i], crc32, poly);
   }
printf("data 4 crc %x size %d\n",crc32, ARRAYSIZE(data4)-2);
   if (match && ((crc32 ^ xorout) & 0xffff) == 0xe406) {
      printf("Match init %x %x\n",crc32_init, xorout);
      if (xorout == 0) 
         printf("****\n");
   }
//printf("Final crc %x\n",crc32);
}
   crc_info.poly = 0x1021;
   crc_info.length = 16;
   crc_info.init_value = 0;
   crc_info.ecc_max_span = 0;

   crc32 = crc64(&data4[2], 7, &crc_info);
   printf("Final crc %x %x\n",crc32, crc32 ^ 0x184f);
   //int data[] = { 0, 0, 0xc2, 0, 0, 0, 0, 0x80, 0, 0x1c, 0x64, 0x00, 0xcf };
   crc32 = 0xffffffff;
   //crc32 = 0;
   for (i = ARRAYSIZE(data) - 1; i >= 0; i--) {
      crc32 = calc_crc32r(data[i], crc32, poly);
      printf("i %d %02x crc %08llx %08llx\n", i, data[i], ~crc_revbits(crc32, 32),
             crc_revbits(crc32, 32));
   }
   // Since the final revbits didn't need inverting though the CRC value
   // did need inverting it's not the same as my init 0xffffffff example.
   // Zero init and not inverting CRC also gets the correct pattern.

   // This was for seeing how they did calculate it. They init the CRC
   // with 0 and didn't invert the final CRC. The init 0xffffffff is
   // so all zeros data doesn't have a zero CRC.
   int data2[] = { 0xc2, 0, 0, 0, 0, 0x80, 0 };
   crc32 = 0;
   for (i = 0; i < ARRAYSIZE(data2); i++) {
      crc32 = calc_crc32(data2[i], crc32, poly);
      printf("%02x ", data2[i]);
   }
   printf("Header CRC %08x %08x\n", crc32, ~crc32);
}

find_poly()
{
// From {0xa1,0xfe,0x00,0x00,0x00,0x00,0x0f,0x9c,0xdb,0x18},
// make CRC 0x0f9cdb18

uint32_t crc_values[] = 
//   {0x0f9cdb18, 0x0e981299, 0x0d95481a, 0x0b8ffd1c, 0x07ba9710, 0x1fd04308};

{
#if 0
0xd76bc516,
0x866f3f12,
0x7562311e
#endif
#if 0
0xc3356342,
0x92219946,
0x613c974a,
0xc622cad7,
0xc91a3068
//3 41044185
#endif
#if 0 ///
0xffd689be,
0xaed273ba,
0x5ddf7db6,
0xfac1202b,
0xf5f9da94
#endif
#if 0
0xac2e,
0xbc0f, 
0x8c6c, 
0xecaa,
0x2d26,//8 
#endif
#if 1
0x84c0,
0x94e1,
0xa482,
0xc444,
0,
0,
0xb7f1,
0xa7d0,
0x97b3,
0xf775,
#endif
};
#if 0
{
0x5de33014,
0x5ce7f995,
0x5feaa316,
0x59f01610,
0x55c57c1c,
0x4dafa804
};
#endif
#if 0
{
0x0a519399,
0x0b555a18,
0x0858009b,
0x0e42b59d,
0x0277df91,
0x1a1d0b89
};
#endif
#if 0
{
0xc526a614,
0x8926a434,
0x5d26a254,
0xf586a691,
0xa466a71e,
0x07a6a400,
0x4086aa39,
0xcec6b64b 
};
#endif
int i;
uint32_t crc, last_crc;
//uint8_t bytes[] = {0xa1,0xfe,0x00,0x00,0x00,0x00,0x0f,0x9c,0xdb,0x18};
//uint8_t bytes[] = {0xa1,0xfe,0x00,0x00,0x00,0x00,0x0f,0x9c,0xdb,0x18};
//uint8_t bytes[] = {0xa1,0xfe,0x00,0x00,0x00,0x00,0x0f^0xff,0x9c^0xff,0xdb^0xff,0x18^0xff};
uint8_t bytes[] = {
0x6a,0x29,0x00,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,
0x76,0x76,0x76
};
uint32_t poly, crc32;

#if 0
last_crc = 1;
for (i = 1; i < ARRAYSIZE(crc_values); i++) {
   crc = crc_values[0] ^ crc_values[i];
   printf("%d crc %08x last %08x\n", i, crc, last_crc);
   if (i >= 2) {
     printf("%d %08x\n", i, crc ^ (last_crc << 1));
   }
   last_crc = crc;
}
#else
for (i = 1; i < ARRAYSIZE(crc_values); i++) {
    crc = crc_values[i] ^ crc_values[i-1];
    printf("%d %08x %x\n", i, crc, crc_values[i-1]);
   if (i >= 2) {
     printf("%d %08x\n", i, crc ^ (last_crc << 1));
   }
   last_crc = crc;
}
#endif
return;
crc = 0xffff;
poly = 0x8005;
for (i = 0; i < ARRAYSIZE(bytes); i++) {
//for (i = 0; i < 6; i++) {
  printf("iii %d %02x %08x\n",i, bytes[i], crc);
}
crc = 0;
for (i = ARRAYSIZE(bytes)-1; i > ARRAYSIZE(bytes)-5; i--) {
  crc = (crc >> 8) | (bytes[i] << 24);
}
printf("crc %08x\n",crc);
for (i = ARRAYSIZE(bytes)-5; i >= 0; i--) {
  crc = calc_crc32rb(bytes[i], crc,  poly);
  printf("ir %d %02x %08x\n",i, bytes[i], crc);
}
//crc = 0x2605fb9c;
//crc = 0xa5074166;
for (i = 0; i < ARRAYSIZE(bytes); i++) {
  crc = calc_crc32(bytes[i], crc,  poly);
  printf("i %d %02x %08x\n",i, bytes[i], crc);
}

   crc32 = 0;
   for (i = ARRAYSIZE(bytes) - 1; i >= 0; i--) {
      crc32 = calc_crc32r(bytes[i], crc32, poly);
      printf("i %d %02x crc %08llx %08llx\n", i, bytes[i], ~crc_revbits(crc32, 32),
             crc_revbits(crc32, 32));
   }
exit(1);
}

