#include "presto.h"
#include "multibeam.h"

int skip_to_multibeam_rec(FILE * infile, long rec)
/* This routine skips to the record 'rec' in the input file */
/* *infile.  *infile contains 1 bit digitized data from the */
/* multibeam receiver at Parkes                             */
/* Returns the record that was skipped to.                  */
{
  chkfileseek(infile, rec, RECLEN, SEEK_SET);
  return rec;
}


int read_multibeam_rec(FILE * infile, multibeam_tapehdr * hdr, \
		       unsigned char *data)
/* This routine reads a single multibeam record from          */
/* the input file *infile which contains 1 bit digitized data */
/* from the multibeam correlator at Parkes.                   */
/* Length of a multibeam record is 640 bytes for the header   */
/* plus 48k of data = 49792 bytes.                            */
{
  static long lastctr = 0;
  long currentctr;
  char tmp[50];

  /* Read the header */

  if (fread(hdr, sizeof(unsigned char), HDRLEN, infile) != HDRLEN) {
    if (feof(infile)) {
      return 0;
    } else {
      printf("\nProblem reading header from multibeam data file.\n");
      printf("Exiting\n");
    }
  }
  /* Verify that the records are in order and all present */

  sprintf(tmp, " %.8s ", hdr->blk_cntr);
  currentctr = strtol(tmp, NULL, 10);
  /*
  if (currentctr - lastctr != 1) {
    printf("\nError:  Raw data file is missing a record between\n");
    printf("        blocks %ld and %ld.  Exiting.\n", lastctr, currentctr);
    exit(1);
  }
  */
  lastctr = currentctr;

  /* Read the raw data */

  if (fread(data, sizeof(unsigned char), DATLEN, infile) != DATLEN) {
    if (feof(infile)) {
      return 0;
    } else {
      printf("\nProblem reading raw data from multibeam data file.\n");
      printf("Exiting\n");
    }
  }
  return 1;
}


int read_multibeam(FILE * infile, float *data, long numpts,
		   double *dispdelays, long numchan)
/* This routine reads a numpts record with numchan each from   */
/* the input file *infile which contains 1 bit digitized data  */
/* from the multibeam correlator at Parkes.                    */
/* It returns the number of points read.                       */
{
  static unsigned char raw[DATLEN], *ptr;
  static int firsttime = 1, recsize = 0, order = 0;
  static float *rawdata1, *rawdata2, *currentdata, *lastdata, *tmp;
  static long worklen = 0;
  static multibeam_tapehdr hdr;
  long i;

  if (firsttime) {
    recsize = numchan / 8;
    worklen = DATLEN * 8;
    /*    raw = gen_bvect(recsize * numpts); */

    /* Create our data storage for the raw floating point data */

    rawdata1 = gen_fvect(worklen);
    rawdata2 = gen_fvect(worklen);
    for (i = 0; i < worklen; i++) {
      rawdata1[i] = 0.0;
      rawdata2[i] = 0.0;
    }

    firsttime = 0;

    /* Read the first multibeam record to cope with end effects */

    if (!read_multibeam_rec(infile, &hdr, raw)) {
      printf("Problem reading the raw data file.\n\n");
      free(rawdata1);
      free(rawdata2);
      return 0;
    }
    /* If order is negative, then the data has been recorded with */
    /* high frequencies first.                                    */

    order = (int) (strtod(hdr.chanbw[1], NULL) * 10000.0);
    order /= abs(order);

    /* Initialize our data pointers */

    currentdata = rawdata1;
    lastdata = rawdata2;

    /* Convert the data to floats */

    ptr = raw;
    for (i = 0; i < numpts; i++, ptr += recsize) {
      convert_multibeam_point(ptr, currentdata + i * numchan, numchan);
    }

    /* Swap our data pointers */

    tmp = currentdata;
    currentdata = lastdata;
    lastdata = tmp;

  }
  /* Read a multibeam record */

  if (!read_multibeam_rec(infile, &hdr, raw)) {
    for (i = 0; i < worklen; i++)
      currentdata[i] = 0.0;
    dedisp(rawdata1, lastdata, numpts, dispdelays, numchan, \
	   data, order);
    free(rawdata1);
    free(rawdata2);
    return 0;
  }
  /* Convert the data to floats */

  ptr = raw;
  for (i = 0; i < numpts; i++, ptr += recsize) {
    convert_multibeam_point(ptr, currentdata + i * numchan, numchan);
  }

  /* De-disperse the data */

  dedisp(currentdata, lastdata, numpts, dispdelays, numchan, \
	 data, order);

  /* Swap our data pointers */

  tmp = currentdata;
  currentdata = lastdata;
  lastdata = tmp;

  /* Return the number of points we actually read */

  return numpts;
}


int read_mb_chan_to_vecs(FILE * infile, float *data,
			 long numpts, long numchan)
/* This routine reads a numpts record with numchan each from   */
/* the input file *infile which contains 1 bit digitized data  */
/* from the multibeam correlator at Parkes.  It stores the     */
/* data in a vector *data of length numchan*numpts.  The low   */
/* frequency channel is stored first, then the next highest    */
/* freq etc, with numpts floating points per frequency.        */
/* It returns the number of points (numpts) read if successful */
/* or 0 if unsuccessful.                                       */
{
  static unsigned char raw[DATLEN], *ptr;
  static int firsttime = 1, recsize = 0, order = 0;
  static float *rawdata = NULL, *tmp = NULL;
  static long worklen = 0;
  static multibeam_tapehdr hdr;
  long i, j;

  if (firsttime) {
    recsize = numchan / 8;
    worklen = numchan * numpts;
    /*    raw = gen_bvect(recsize * numpts); */

    /* Create our data storage for the raw floating point data */

    rawdata = gen_fvect(worklen);
    tmp = gen_fvect(worklen);

    firsttime = 0;

    /* Read the first multibeam record to cope with end effects */

    if (!read_multibeam_rec(infile, &hdr, raw)) {
      printf("Problem reading the raw data file.\n\n");
      free(rawdata);
      return 0;
    }

    /* If order is negative, then the data has been recorded with */
    /* high frequencies first.                                    */

    order = (int) (strtod(hdr.chanbw[1], NULL) * 10000.0);
    order /= abs(order);

  } else {

    /* Read a multibeam record */
    
    if (!read_multibeam_rec(infile, &hdr, raw)) {
      printf("Problem reading the raw data file.\n\n");
      free(rawdata);
      return 0;
    }

  }    

  /* Convert the data to floats */
  
  ptr = raw;
  for (i = 0; i < numpts; i++, ptr += recsize) {
    convert_multibeam_point(ptr, rawdata + i * numchan, numchan);
  }

  /* Change ordering so that all of the data for an */
  /* individual frequency is grouped together.      */

  if (order > 0) {
    
    for (i = 0 ; i < numchan ; i++) {
      for (j = 0 ; j < numpts ; j++) {
	data[i * numpts + j] = rawdata[j * numchan + i];
      }
    }

  } else {

    for (i = 0 ; i < numchan ; i++) {
      for (j = 0 ; j < numpts ; j++) {
	data[i * numpts + j] = rawdata[j * numchan + numchan - i - 1];
      }
    }

  }
  
  /* Return the number of points we actually read */
  
  return numpts;
}


void multibeam_hdr_to_inf(multibeam_tapehdr * hdr, infodata * idata)
/* Convert appropriate Multibeam header portions to make */
/* a basic '.inf' file                                   */
{
  double tmp1, tmp2;
  char ctmp[100];
  long ptsperrec = 0;

  sprintf(idata->object, "%.16s", hdr->pname);
  sscanf(hdr->ra_start, \
	 "%3d:%2d:%9lf", &idata->ra_h, &idata->ra_m, &idata->ra_s);
  sscanf(hdr->dec_start, \
	 "%3d:%2d:%9lf", &idata->dec_d, &idata->dec_m, &idata->dec_s);
  strcpy(idata->telescope, "Parkes");
  strcpy(idata->instrument, "Multibeam (center beam)");
  sscanf(hdr->nchan[0], "%4d", &idata->num_chan);
  ptsperrec = DATLEN / (idata->num_chan / 8);
  sscanf(hdr->samp_int[0], "%12lf", &idata->dt);
  idata->dt *= 0.001;
  sscanf(hdr->nblk_read, "%8lf", &tmp1);
  sprintf(ctmp, " %.8s ", hdr->obs_time);
  tmp2 = strtod(ctmp, NULL);
  if (tmp1) {
    idata->N = ptsperrec * tmp1;
  } else {
    idata->N = tmp2 / idata->dt;
  }
  tmp1 = strtod(hdr->chanbw[0], NULL);
  idata->chan_wid = fabs(tmp1);
  idata->freqband = idata->num_chan * idata->chan_wid;
  idata->freq = strtod(hdr->freq_ch1[0], NULL);
  if (tmp1 < 0.0) {
    idata->freq -= idata->freqband - idata->chan_wid;
  }
  idata->fov = 1.2 * SOL * 3600.0 / (1000000.0 * idata->freq * 64 * DEGTORAD);
  idata->mjd_i = strtol(hdr->mjd, NULL, 10);
  sscanf(hdr->ut_start, "%lf:%lf:%lf", &idata->mjd_f, &tmp1, &tmp2);
  idata->mjd_f = (idata->mjd_f + (tmp1 + tmp2 / 60.0) / 60.0) / 24.0;
  sprintf(ctmp, " %.8s ", hdr->blk_cntr);
  tmp1 = strtod(ctmp, NULL) - 1.0;
  idata->mjd_f += (tmp1 * ptsperrec * idata->dt) / SECPERDAY;
  if (idata->mjd_f >= 1.0){
    idata->mjd_f -= 1.0;
    idata->mjd_i += 1;
  }
  idata->bary = 0;
  idata->numonoff = 1;
  idata->onoff = (double *)malloc(2*sizeof(double));
  idata->onoff[0] = 0;
  idata->onoff[1] = idata->N - 1;
  strcpy(idata->band, "Radio");
  strcpy(idata->analyzer, "--");
  strcpy(idata->observer, "--");
  sprintf(idata->notes, "Topo UT Date & Time at file start = %.8s, %.16s  From tape %.6s  file #%.4s  block #%.8s  Comment: %.64s\n", hdr->date, hdr->ut_start, hdr->tape_lbl, hdr->file_cntr, hdr->blk_cntr, hdr->comment);

}



void print_multibeam_hdr(multibeam_tapehdr * hdr)
/* Output in human readable form a multibeam header. */
{
  printf("\n");
  printf(" Program name                                      = %.6s\n", \
	 hdr->program);
  printf(" Version number 'vv.v'                             = %.4s\n", \
	 hdr->version);
  printf(" Tape label                                        = %.6s\n", \
	 hdr->tape_lbl);
  printf(" File number on tape                               = %.4s\n", \
	 hdr->file_cntr);
  printf(" Block counter per beam in file                    = %.8s\n", \
	 hdr->blk_cntr);
  printf(" UT Date 'yymmdd  '                                = %.8s\n", \
	 hdr->date);
  printf(" MJD at 00h UT                                     = %.8s\n", \
	 hdr->mjd);
  printf(" UT at file start  ' hh:mm:ss.ssssss'              = %.16s\n", \
	 hdr->ut_start);
  printf(" LST at file start ' hh:mm:ss.ss'                  = %.12s\n", \
	 hdr->lst_start);
  printf(" Coord system ' 4' = Galactic, ' 5' = J2000        = %.2s\n", \
	 hdr->coord_type);
  printf(" RA at file start  ' hh:mm:ss.ssss'                = %.16s\n", \
	 hdr->ra_start);
  printf(" Dec at file start '-dd:mm:ss.sss'                 = %.16s\n", \
	 hdr->dec_start);
  printf(" Galactic long at file start (deg) 'ddd.dddd'      = %.8s\n", \
	 hdr->gl_start);
  printf(" Galactic lat. at file start (deg) '-dd.dddd'      = %.8s\n", \
	 hdr->gb_start);
  printf(" FA, PA or GPA (deg) '-ddd.ddd'                    = %.8s\n", \
	 hdr->fangle);
  printf(" Obs length 'ssss.sss'                             = %.8s\n", \
	 hdr->obs_time);
  printf(" Comment                                           = %.64s\n", \
	 hdr->comment);
  printf(" Number of filter systems, ' 1' or ' 2'            = %.2s\n", \
	 hdr->nfilter);
  printf(" Channel incrt (MHz, -ve if inverted) '-b.bbbbb'   = %.8s\n", \
	 hdr->chanbw[0]);
  printf(" Nr of channels in each filter system              = %.4s\n", \
	 hdr->nchan[0]);
  printf(" RF of first channel centre 'ffffff.fffff'         = %.12s\n", \
	 hdr->freq_ch1[0]);
  printf(" Samp intval in ms (1 = 2 always) 'mm.mmmmmmmmm'   = %.12s\n", \
	 hdr->samp_int[0]);
  printf(" Samp per group (For backwards compat., now = 1)   = %.4s\n", \
	 hdr->samp_grp[0]);
  printf(" Samples per block (Was groups per block)          = %.8s\n", \
	 hdr->samp_blk);
  printf(" Seconds per tape block per beam 'ss.sssss'        = %.8s\n", \
	 hdr->blk_sec);
  printf(" 0 = none, 1 = fixed FA, 2 = fixed PA or GPA       = %.2s\n", \
	 hdr->fdctrl);
  printf(" Data type code ('2'=pol, '3'=normal, '5'=dedisp)  = %.1s\n", \
	 hdr->data[0]);
  printf(" UT at blk start ' hh:mm:ss.ssssss'                = %.16s\n", \
	 hdr->ut_blk);
  printf(" LST at blk start ' hh:mm:ss.ss'                   = %.12s\n", \
	 hdr->lst_blk);
  printf(" RA at blk start ' hh:mm:ss.ssss'                  = %.16s\n", \
	 hdr->ra_blk);
  printf(" Dec at blk start '-dd:mm:ss.sss'                  = %.16s\n", \
	 hdr->dec_blk);
  printf(" Gal long. at blk start 'ddd.dddd'                 = %.8s\n", \
	 hdr->gl_blk);
  printf(" Gal lat at blk start '-dd.dddd'                   = %.8s\n", \
	 hdr->gb_blk);
  printf(" Zenith angle at blk start (deg) 'ddd.dddd'        = %.8s\n", \
	 hdr->zen_blk);
  printf(" Azimuth angle at blk start (deg) 'ddd.dddd'       = %.8s\n", \
	 hdr->az_blk);
  printf(" Attenuator settings (A,B) for Sys 1,2 'dd.d'      = %.4s\n", \
	 hdr->atten[0]);
  printf(" Total powers for 32-channel groups 'iiii'         = %.4s\n", \
	 hdr->tpower[0]);
  printf(" Nr of tape blocks in disk file                    = %.8s\n", \
	 hdr->nblk_read);
  printf(" Scan rate in `long' (deg/min) '-r.rrrrr'          = %.8s\n", \
	 hdr->scan_ratel);
  printf(" Scan rate in `lat'  (deg/min) '-r.rrrrr'          = %.8s\n", \
	 hdr->scan_rateb);
  printf(" Total number of beams                             = %.4s\n", \
	 hdr->nbeam);
  printf(" Beam number                                       = %.4s\n", \
	 hdr->ibeam);
  printf(" Psrname (B or J) or pointing name                 = %.16s\n", \
	 hdr->pname);
  printf(" Config file name                                  = %.8s\n", \
	 hdr->cfg_file);
  printf(" Schedule file name                                = %.8s\n", \
	 hdr->schd_file);
  printf(" Nr of bits per sample                             = %.2s\n", \
	 hdr->nbits[0]);
  printf(" DM for dedispersion 'ddddd.dd'                    = %.8s\n", \
	 hdr->ddm);
  printf(" Nr of channels per dedispersed band               = %.4s\n", \
	 hdr->nddch[0]);
  printf(" Tel status 'k0': On posn k(0-9)                   = %.2s\n", \
	 hdr->move);
  printf("            '01': On grid posn\n");
  printf("            '02': Off position\n");
  printf("            '03': GC scan\n");
  printf("            '04': SC scan\n");
  printf(" Pointing error (arcmin) 'mmm.mm'                  = %.6s\n", \
	 hdr->pnterr);
  printf(" Dedisp type: '  ': Norm, ' T': Tree, ' D': PDM    = %.2s\n", \
	 hdr->tree);
  printf(" Filter system for disk file                       = %.2s\n", \
	 hdr->ifilter);
  printf(" Telescope ID (as in tel.def)                      = %.10s\n", \
	 hdr->telid);
  printf(" Parallactic angle (deg) '-ddd.ddd'                = %.8s\n", \
	 hdr->pangle);
  printf(" Nr of samples per beam switch                     = %.8s\n", \
	 hdr->bsw);
  printf(" Cal cycle period in samples                       = %.4s\n", \
	 hdr->cal_cycle);
  printf(" Spare                                             = %.22s\n", \
	 hdr->spare);
  printf("\n");
}


void convert_multibeam_point(unsigned char *rec, float *data, \
			     int numchan)
/* This routine converts 1 bit digitized data with      */
/* 'numchan' channels to an array of 'numchan' floats.  */
{
  int ii, jj;

  for(ii = 0, jj = 0; ii < numchan / 8; ii++, jj+=8){
    data[jj] = rec[ii] & 0x80 ? 1.0 : 0.0;
    data[jj+1] = rec[ii] & 0x40 ? 1.0 : 0.0;
    data[jj+2] = rec[ii] & 0x20 ? 1.0 : 0.0;
    data[jj+3] = rec[ii] & 0x10 ? 1.0 : 0.0;
    data[jj+4] = rec[ii] & 0x08 ? 1.0 : 0.0;
    data[jj+5] = rec[ii] & 0x04 ? 1.0 : 0.0;
    data[jj+6] = rec[ii] & 0x02 ? 1.0 : 0.0;
    data[jj+7] = rec[ii] & 0x01 ? 1.0 : 0.0;
  }
}