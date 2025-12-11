// Shannon Tuohey
#include <stdio.h>    // for I/O functions
#include <stdlib.h>   // for exit()
#include <string.h>   // for string functions
#include <time.h>     // for time functions

int i, j;

unsigned short temp, inst, addr;
char buf[300];

FILE *infile;
FILE *outfile;
char c, *p, letter;

unsigned short mca[65536];
int mcaindex = 0;

unsigned short start;
int gotstart;

unsigned short Gadd[1000];
char *Gptr[1000];
int Gindex;

unsigned short Eadd[1000];
char *Eptr[1000];
int Eindex;

unsigned short eadd[1000];
char * eptr[1000];
int eindex;

unsigned short Aadd[1000];
int Amodadd[1000];
int Aindex;

unsigned short Vadd[1000];
char *Vptr[1000];
int Vindex;

time_t timer;

int main(int argc,char *argv[])
{
   if (argc < 2)
   {
        printf("Wrong number of command line arguments\n");
        printf("Usage: l <obj module name1> <obj module name2> ... \n");
        exit(1);
   }

   // display your name, command line args, time
   time(&timer);      // get time
   printf("Shannon Tuohey   %s %s   %s", 
           argv[0], argv[1], asctime(localtime(&timer)));

   //================================================================
   // Step 1:
   // For each module, store header entries into tables with adjusted
   // addresses and store machine code in mca (the machine code array).

   
   mcaindex = 0;

   for (i = 1; i < argc; i++)
   {
      infile = fopen(argv[i], "rb");
      if (!infile)
      {
         printf("Cannot open %s\n", argv[i]);
         exit(1);
      }
      printf("Linking %s\n", argv[i]);
      letter = fgetc(infile);
      if (letter != 'o')
      {
         printf("Not a linkable file\n");
         exit(1);
      }
      while (1)
      {
         letter = fgetc(infile);
         if (letter == 'C')
            break;
         else
         if (letter == 'S')
         {
            if (fread(&addr, 2, 1, infile) != 1) // addr unsigned short
            {
               printf("Invalid S entry\n");
               exit(1);
            }
            if (gotstart)
            {
               printf("More than one entry point\n");
               exit(1);
            }
            gotstart = 1;                   // indicate S entry processed
            start = addr + mcaindex;        // save adjusted address
         }
         else
         if (letter == 'G')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid G entry\n");
               exit(1);
            }
            Gadd[Gindex] = addr + mcaindex; // save adjusted address
            j = 0;
            do                              // get string in G entry
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j = 0;
            while (j < Gindex)     // check for multiple definitions
            {
               if (!strcmp(buf, Gptr[j]))
               {
                  printf("Multiple defs of global var %s\n", buf);
                  exit(1);
               }
               else
                  j++;
            }
            Gptr[Gindex++] = strdup(buf);   // save string
         }
         else
         if (letter == 'E')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid E entry\n");
               exit(1);
            }
            Eadd[Eindex] = addr + mcaindex; // save adjusted address
            j = 0;
            do                              // get string in E entry
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j=0;
            while (j < Eindex)     // check for multiple definitions
            {
               if (!strcmp(buf, Eptr[j]))
               {
                  printf("Multiple defs of external var %s\n", buf);
                  exit(1);
               }
               else
                  j++;
            }
            Eptr[Eindex++] = strdup(buf);   // save string
         }

         else
         if (letter == 'e')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid e entry\n");
               exit(1);
            }
            eadd[eindex] = addr + mcaindex; // save adjusted address
            j = 0;
            do                              // get string in e entry
            {               
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j=0;
            while (j < eindex)     // check for multiple definitions
            {
               if (!strcmp(buf, eptr[j]))
               {
                  printf("Multiple defs of external var %s\n", buf);
                  exit(1);
               }
               else
                  j++;
            }
            eptr[eindex++] = strdup(buf);   // save string
         }
         else
         if (letter == 'V')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid V entry\n");
               exit(1);
            }
            Vadd[Vindex] = addr + mcaindex; // save adjusted address
            j = 0;
            do                              // get string in V entry
            {
               letter = fgetc(infile);
               buf[j++] = letter;
            } while (letter != '\0');
            j=0;
            while (j < Vindex)     // check for multiple definitions
            {
               if (!strcmp(buf, Vptr[j]))
               {
                  printf("Multiple defs of variable %s\n", buf);
                  exit(1);
               }
               else
                  j++;
            }
            Vptr[Vindex++] = strdup(buf);   // save string
         }
         else
         if (letter == 'A')
         {
            if (fread(&addr, 2, 1, infile) != 1)
            {
               printf("Invalid A entry\n");
               exit(1);
            }
            Aadd[Aindex] = addr + mcaindex; // save adjusted address
            Amodadd[Aindex++] = mcaindex;     // save module's code start address


         }
         else
         {
            printf("Invalid header entry %c in %s\n", letter, argv[i]);
            exit(1);
         }
      }
      // add machine code to machine code array
      while(fread(&inst, 2, 1, infile))
      {
         mca[mcaindex++] = inst;
      }


      fclose(infile);

      
   }


   //================================================================
   // Step 2: Adjust external references

   // handle E references
   for (i = 0; i < Eindex; i++)
   {
      for (j = 0; j < Gindex; j++)
         if(!strcmp(Eptr[i], Gptr[j]))
            break;
      if (j >= Gindex)
      {
         printf("%s is an undefined external reference", Eptr[i]);
         exit(1);
      }
      mca[Eadd[i]] = (mca[Eadd[i]] & 0xf800) | //masks the opcode leaving 11 bits as 0 
                     ((mca[Eadd[i]] + Gadd[j] - Eadd[i] - 1) & 0x7ff); //masks last 11 bits
   }
   // handle e entries
   for (i = 0; i < eindex; i++)
   {
      for (j = 0; j < Gindex; j++)
         if(!strcmp(eptr[i], Gptr[j]))
            break;
      if (j >= Gindex)
      {
         printf("%s is an undefined external e reference", eptr[i]);
         exit(1);
      }
      mca[eadd[i]] = (mca[eadd[i]] & 0xfe00) | //preserves opcode leaving 7 bits as 0
                     ((mca[eadd[i]] + Gadd[j] - eadd[i] - 1) & 0x1ff); //masks last 9 bits
   }

   // handle V entries
   for (i = 0; i < Vindex; i++)
   {
      for (j = 0; j < Gindex; j++)
         if(!strcmp(Vptr[i], Gptr[j]))
            break;
      if (j >= Gindex)
      {
         printf("%s is an undefined external V reference", Vptr[i]);
         exit(1);
      }
      mca[Vadd[i]] = mca[Vadd[i]]+ Gadd[j]; //add address directly
   }

   //================================================================
   // Step 3: Handle A entries

   for (i = 0; i < Aindex; i++)
   {
      mca[Aadd[i]] = (mca[Aadd[i]] +  Amodadd[i]) & 0xffff; // add module start address directly
   }
   //================================================================
   // Step 4: Write out executable file

   outfile = fopen("link.e", "wb");
   if (!outfile)
   {
      printf("Cannot open output file link.e\n");
      exit(1);
   }

   // Write out file signature
   fwrite("o", 1, 1, outfile);
   
   printf("Creating executable file link.e\n");
   // Write out start entry if there is one
   if (gotstart)
   {
      fwrite("S", 1, 1, outfile);
      fwrite(&start, 2, 1, outfile);
   }
   // Write out G entries
   for (i = 0; i < Gindex; i++)
   {
      fwrite("G", 1, 1, outfile);
      fwrite(Gadd + i, 2, 1, outfile);
      fprintf(outfile, "%s", Gptr[i]);
      fwrite("", 1, 1, outfile);
   }
   // Write out V entries as A entries
   for (i = 0; i < Vindex; i++)
   {
      fwrite("A", 1, 1, outfile);
      fwrite(Vadd + i, 2, 1, outfile);
   }
   // Write out A entries
   for (i = 0; i < Aindex; i++)
   {                        
      fwrite("A", 1, 1, outfile);
      fwrite(Aadd + i, 2, 1, outfile);
   }
   // Terminate header
   fwrite("C", 1, 1, outfile);
      
   // Write out code
   for (i = 0; i < mcaindex; i++)
   {
      fwrite(mca + i, 2, 1, outfile);
   }
   fclose(outfile);

    
}
