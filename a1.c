// Shannon Tuohey
#include <stdio.h>    // for I/O functions
#include <stdlib.h>   // for exit()
#include <string.h>   // for string functions
#include <ctype.h>    // for isspace(), tolower()
#include <time.h>     // for time functions

FILE *infile, *outfile;
short pcoffset9, pcoffset11, imm5, imm9, offset6;
unsigned short symadd[500], macword, dr, sr, sr1, sr2, baser, trapvec, eopcode;
char outfilename[100], linesave[100], buf[100], *symbol[500], *p1, *p2, *cp,
     *mnemonic, *o1, *o2, *o3, *label;
int stsize, linenum, rc, loc_ctr, num;
time_t timer;


short int mystrcmpi(const char *p, const char *q) 
{
   char a, b;
   while (1) 
   {
      a = tolower(*p); b = tolower(*q);
      if (a != b) return a-b;
      if (a == '\0') return 0;
      p++; q++;
   }
   return 0;
}

short int mystrncmpi(const char *p, const char *q, int n) 
{
   char a, b;
   int i;
   for (i = 1; i <= n; i++) 
   {
      a = tolower(*p); b = tolower(*q);
      if (a != b) return a-b;
      if (a == '\0') return 0;
      p++; q++;
   }
   return 0;
}

void error(char *p)
{
   
   fprintf(stderr, "Error: %s at line %d\n >> %s\n", p, linenum, linesave);
   if (infile) fclose(infile);
   if (outfile) fclose(outfile);


   exit(1);
}
int isreg(char *p)
{
    if (p[0] == 'r' && isdigit(p[1]) && (p[1]-'0') <= 7 && p[2] == '\0'){
        return 1;
    }
    else{
        return 0;
    }
   
}
unsigned short getreg(char *p)              
{
    if (!isreg(p)){
        error("Not a register name");
    }
    else{
        return p[1] - '0';
    }
    return -1;
  
}

unsigned short getadd(char *p)
{
    for (int i = 0; i< stsize; i++){
        if(!strcmp(symbol[i], p)){
            return symadd[i];
        }
    }
    error("Undefined symbol");
    return -1;

}



int main(int argc,char *argv[])
{
   if (argc != 2)
   {
      printf("Wrong number of command line arguments\n");
      printf("Usage: a1 <input filename>\n");
      exit(1);
   }

   // display your name, command line args, time
   time(&timer);      // get time
   printf("Shannon Tuohey   %s %s   %s", 
           argv[0], argv[1], asctime(localtime(&timer)));

   infile = fopen(argv[1], "r");
   if (!infile)
   {
      printf("Cannot open input file %s\n", argv[1]);
      exit(1);
   }

   // construct output file name
   strcpy(outfilename, argv[1]);        // copy input file name
   p1 = strrchr(outfilename, '.');      // search for period in extension
   if (p1)                              // name has period
   {
#ifdef _WIN32                           // defined only on Windows systems
      p2 = strrchr(outfilename, '\\' ); // compiled if _WIN32 is defined
#else
      p2 = strrchr(outfilename, '/');   // compiled if _WIN32 not defined
#endif
      // can p2 < p1 in following if statement be eliminated by 
      // using strchr(p1, ...) instead of strrchr(outfilename, ...) 
      // in the preceding assignments to p2?
      if (!p2 || p2 < p1)               // input file name has extension?
         *p1 = '\0';                    // null out extension
   }
   strcat(outfilename, ".e");           // append ".e" extension

   outfile = fopen(outfilename, "wb");
   if (!outfile)
   {
      printf("Cannot open output file %s\n", outfilename);
      exit(1);
   }

   loc_ctr = linenum = 0;       // initialize, not required because global
    fwrite("oC", 2, 1, outfile); // output empty header

    



   // Pass 1
   printf("Starting Pass 1\n");
   while (fgets(buf, sizeof(buf), infile))
   {
      linenum++;  // update line number
      cp = buf;
      while (isspace(*cp))
         cp++;
      if (*cp == '\0' || *cp ==';')  // if line all blank, go to next line
         continue;
         strcpy(linesave, buf);  // save line for error messages

         // --- Tokenize for label ---
         // Remove trailing comments
      char *comment = strchr(buf, ';');
      if (comment) *comment = '\0';  // terminate string at ';'


         mnemonic = NULL;
         o1 = o2 = o3 = NULL;
         label = NULL;
     
         char *first = strtok(buf, " \r\n\t");
         if (!first) continue;
     
         if (strchr(first, ':')) {
             label = first;
             label[strlen(label)-1] = '\0';  // remove colon
             mnemonic = strtok(NULL, " \r\n\t");  // next token is mnemonic
         } else {
             mnemonic = first;  // no label
         }
     
         o1 = strtok(NULL, " \r\n\t,");
         o2 = strtok(NULL, " \r\n\t,");
         o3 = strtok(NULL, " \r\n\t,");
     
         if (label) {
             for (int i = 0; i < stsize; i++) {
                 if (!strcmp(symbol[i], label)) {
                     error("Duplicate label");
                 }
             }
             symbol[stsize] = strdup(label);
             symadd[stsize++] = loc_ctr;  // assign current loc_ctr to label
         }
     
         if (!mnemonic)
             continue;
     
         if (!mystrcmpi(mnemonic, ".zero")) {
             if (!o1) error("Missing operand for .zero");
             rc = sscanf(o1, "%d", &num);
             if (rc != 1 || num < 1 || num > (65536 - loc_ctr))
                 error("Invalid operand for .zero");
             loc_ctr += num;  // increment by N
         }
         else if (!mystrcmpi(mnemonic, ".word")) {
             loc_ctr++;
         }
         else {
             loc_ctr++;
         }
     
         if (loc_ctr >= 65536)
             error("Program too big");
     }
     

   rewind(infile);

   // Pass 2

   // Before writing any instructions, write the origin (e.g., x3000)
loc_ctr = 0x3000;
short origin = loc_ctr;

   printf("Starting Pass 2\n");
   loc_ctr = linenum = 0;      // reinitialize
   while (fgets(buf, sizeof(buf), infile))
   {
      linenum++;
      
      strcpy(linesave, buf); //save line for error messages 
      cp = buf;
      while (isspace(*cp)){
        cp++; //skip leading spaces
      }
      if (*cp == '\0' || *cp == ';'){
        continue; //skip blank or commented lines
      }


      char *first = strtok(buf, " \r\n\t");
      if (first == NULL) continue;

      if (strchr(first, ':')){
        label = first;
        label[strlen(label)-1] = '\0';
        mnemonic = strtok(NULL, " \r\n\t");
      } else { 
        mnemonic = first;
        label = NULL;
      }
      o1 = strtok(NULL, " \r\n\t,");
      o2 = strtok(NULL, " \r\n\t,");
      o3 = strtok(NULL, " \r\n\t,");
      //TOKENIZE



      if (mnemonic == NULL)
         continue;
      if (!mystrncmpi(mnemonic, "br", 2))    // case sensitive compares
      {
         if (!o1)
         error("Branch missing target label");

         if (!mystrcmpi(mnemonic, "br" ))
            macword = 0x0e00;
         else
         if (!mystrcmpi(mnemonic, "brz" ))
            macword = 0x0000;
         else
         if (!mystrcmpi(mnemonic, "brnz" ))
            macword = 0x0200;
         else
         if (!mystrcmpi(mnemonic, "brn" ))
            macword = 0x0400;
         else
         if (!mystrcmpi(mnemonic, "brp" ))
            macword = 0x0600;
         else
         if (!mystrcmpi(mnemonic, "brlt" ))
            macword = 0x0800;
         else
         if (!mystrcmpi(mnemonic, "brgt" ))
            macword = 0x0a00;
         else
         if (!mystrcmpi(mnemonic, "brc" ))
            macword = 0x0c00;
         else
            error("Invalid branch mnemonic");
         
        
         pcoffset9 = (getadd(o1) - loc_ctr - 1);    // compute pcoffset9
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");
         macword = macword | (pcoffset9 & 0x01ff);  // assemble inst
         fwrite(&macword, 2, 1, outfile);           // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "add"))
      {
         if (!o3)
            error("Missing operand");
         dr = getreg(o1) << 9;   // get and position dest reg number
         sr1 = getreg(o2) << 6;  // get and position srce reg number
         if (isreg(o3)) // is 3rd operand a reg?
         {
            sr2 = getreg(o3);      // get third reg number
            macword = 0x1000 | dr | sr1 | sr2; // assemble inst
         }
         else
         {
            if (sscanf(o3,"%d", &num) != 1)    // convert imm5 field
               error("Bad imm5");
            if (num > 15 || num < -16)
               error("imm5 out of range");
            macword = 0x1000 | dr | sr1 | 0x0020 | (num & 0x1f);
         }
        fwrite(&macword, 2, 1, outfile);      // write out instruction
        loc_ctr++;

      }
      else
      if (!mystrcmpi(mnemonic, "ld" ))
      {
         dr = getreg(o1) << 9;// get and position destination reg number
         pcoffset9 = (getadd(o2) - loc_ctr - 1);
         if (pcoffset9 > 255 || pcoffset9 < -256)
            error("pcoffset9 out of range");
         macword = 0x2000 | dr | (pcoffset9 & 0x1ff);   // assemble inst
         fwrite(&macword, 2, 1, outfile); // write out instruction
         loc_ctr++;
      }

       else
       if (!mystrcmpi(mnemonic, "st")){
         if (!o2){
            error("Branch missing target label");
         }
            sr = getreg(o1) << 9;
            pcoffset9 = (getadd(o2) - loc_ctr - 1);
            if (pcoffset9 > 255 || pcoffset9 < -256) {
            error("pcoffset9 out of range"); }
            macword = 0x3000 | sr | (pcoffset9 & 0x1ff);   // assemble inst
            fwrite(&macword, 2, 1, outfile); // write out instruction
            loc_ctr++;
    
       }
        else
         if (!mystrcmpi(mnemonic, "ldr")){
            if(!o3) {
               error("Missing operand for ldr.");
            }
            dr = getreg(o1) << 9;
            baser = getreg(o2) << 6;
            if (sscanf(o3, "%d", &num) != 1){
               error("Bad offset 6");
            }
            if (num > 31 || num <-32){
               error("offset6 out of range");
            }
            macword = 0x6000 | dr | baser | (num & 0x3f);
            fwrite(&macword, 2, 1, outfile);
            loc_ctr++;
      }
       else
       if (!mystrcmpi(mnemonic, "str")){
            if(!o3){
               error("Missing operand for str");
            }
            sr = getreg(o1) << 9;
            baser = getreg(o2) << 6;
            if(sscanf(o3, "%d", &num) != 1){
               error("Bad offset6");
            }
            if (num > 31 || num <-32){
               error("offset6 out of range");
            }
            macword = 0x7000 | sr | baser | (num & 0x3f);
            fwrite(&macword, 2, 1, outfile);
            loc_ctr++;
       }

       else
       if (!mystrcmpi(mnemonic, "bl")){
            if(!o1){
               error("Missing target label for bl");
            }
               int pcoffset11 = getadd(o1) - loc_ctr - 1;
               if(pcoffset11 > 1023 || pcoffset11 < -1024){
                  error("pcoffset11 out of range");
               }
               macword = 0x4800 | (pcoffset11 &0x07FF);
               fwrite(&macword, 2, 1, outfile);
               loc_ctr++;
            
       }
       else if (!mystrcmpi(mnemonic, "blr")) {
         if (!o1)
             error("Missing base register for blr");
     
         baser = getreg(o1) << 6;
         num = 0;     
         macword = 0x4000 | baser | (num & 0x3f);
         fwrite(&macword, 2, 1, outfile);
         loc_ctr++;
     }
     
      else 
      if (!mystrcmpi(mnemonic, "and")){
        if (!o3){
            error("Missing operand");
        }
        dr = getreg(o1) << 9;   // get and position dest reg number
        sr1 = getreg(o2) << 6;  // get and position srce reg number
         if (isreg(o3)) // is 3rd operand a reg?
         {
            sr2 = getreg(o3);      // get third reg number
            macword = 0x5000 | dr | sr1 | sr2; // 0x5000 is the opcode 5 bc in binary opcode is 0101
         }
         else
         {
         if (sscanf(o3,"%d", &num) != 1)    // convert imm5 field
               error("Bad imm5");
            if (num > 15 || num < -16)
               error("imm5 out of range");
            macword = 0x5000 | dr | sr1 | 0x0020 | (num & 0x1f); //bit 5 = 1 for imm5
         }
         fwrite(&macword, 2, 1, outfile);      // write out instruction
         loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "not")){
            if (!o2){
                error("Missing operand");
            }
            dr = getreg(o1) << 9;   // get not position dest reg number
            sr1 = getreg(o2) << 6;  // get not position srce reg number
            macword = 0x9000 | dr | sr1; // 0x9000 is the opcode 9 .  0x003F is lower 6 bits
            fwrite(&macword, 2, 1, outfile);
            loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "jmp" ))     // also ret instruction
      {
         baser = getreg(o1) << 6;        // get reg number and position it
         if (o2)                         // offset6 specified?
         {
            if (sscanf(o2,"%d", &num) != 1)    // convert offset6 field
               error("Bad offset6");
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else
            num = 0;                           // offset6 defaults to 0
         // combine opcode, reg number, and offset6
         macword = 0xc000 | baser | num;       
         fwrite(&macword, 2, 1, outfile);  // write out instruction
         loc_ctr++;
      }
      else
      if (!mystrcmpi(mnemonic, "ret" ))     // also ret instruction
      {
         
         baser = 7 <<6;
         if (o1) {
            if(sscanf(o1, "%d", &num) != 1){
               error("Bad offset 6");
            }
            if (num > 31 || num < -32)
               error("offset6 out of range");
         }
         else{
            num = 0;  
            }
         macword = 0xc000 | baser | (num & 0x3f);       
         fwrite(&macword, 2, 1, outfile);  // write out instruction
         loc_ctr++;
         
         }

      

      else
      if (!mystrcmpi(mnemonic, "lea")){
         if(!o2) {
            error("Missing operand for lea");
         }
         dr = getreg(o1) << 9;
         pcoffset9 = (getadd(o2) - loc_ctr - 1);
         if (pcoffset9 > 255 || pcoffset9 < -256){
            error("pcoffset9 out of range");
         }
         macword = 0xE000 | dr | (pcoffset9 & 0x1ff);
         fwrite(&macword, 2, 1, outfile);
         loc_ctr++;
      }
      
      
      else 
      if (!mystrcmpi(mnemonic, "halt")){
        macword = 0xF000; //trap opcode, then keeping lower 8 bits. 
        fwrite(&macword, 2, 1, outfile);
        loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "nl")){
        macword = 0xF001;
        fwrite(&macword, 2, 1, outfile);
        loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, "dout")){
         dr = getreg(o1) << 9;
         macword = 0xf002 | dr;
        fwrite(&macword, 2, 1, outfile);
        loc_ctr++;
      }

      else
      if(!mystrcmpi(mnemonic, ".word")){
        if (!o1){
            error("Missing operand for .word");
        }
       if(sscanf(o1, "%d", &num) == 1){//if o1 is a number
            macword = num;
       }
       else{ //if o1 is a string
            macword = getadd(o1);
       }
       fwrite(&macword, 2, 1, outfile);
       loc_ctr++;
      }

      else
      if (!mystrcmpi(mnemonic, ".zero"))
      {
         if(!o1){
            error("Missing operand for .zero");
         }
         sscanf(o1, "%d", &num);             // get size of block
         macword = 0;
         while (num--) {                      // write out a block of zeros
            fwrite(&macword, 2, 1, outfile);
            loc_ctr++;
         }
      }
      else
         error("Invalid mnemonic or directive");

   }
   // Close files.
   fclose(infile);
   fclose(outfile);
   printf("Assembly Complete. Output written to %s\n", outfilename);
   return 0;
}

