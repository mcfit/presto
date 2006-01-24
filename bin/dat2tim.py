#!/usr/bin/env python
import scipy.io, struct, presto, sys

telescope_ids = {"Fake": 0, "Arecibo": 1, "Ooty": 2, "Nancay": 3,
                 "Parkes": 4, "Jodrell": 5, "GBT": 6, "GMRT": 7,
                 "Effelsberg": 8}

machine_ids = {"FAKE": 0, "PSPM": 1, "Wapp": 2,"AOFTM": 3,
               "BCPM1": 4, "OOTY": 5, "SCAMP": 6, "GBT Pulsar Spigot": 7}

def prep_string(string):
    return struct.pack('i', len(string))+string

def prep_double(name, value):
    return prep_string(name)+struct.pack('d', float(value))

def prep_int(name, value):
    return prep_string(name)+struct.pack('i', int(value))

def infodata_to_sigproc_header(inf):
    hdr = prep_string("HEADER_START")
    hdr += prep_int("telescope_id", telescope_ids[inf.telescope.strip()])
    hdr += prep_int("machine_id", machine_ids[inf.instrument.strip()])
    hdr += prep_int("data_type", 2)
    hdr += prep_string("source_name")
    hdr += prep_string(inf.object)
    hdr += prep_int("barycentric", 0)
    hdr += prep_int("pulsarcentric", 0)
    hdr += prep_double("src_raj", inf.ra_h*1000.0 + inf.ra_m*100.0 + inf.ra_s)
    hdr += prep_double("src_dej", inf.dec_d*1000.0 + inf.dec_m*100.0 + inf.dec_s)
    hdr += prep_int("nifs", 1)
    hdr += prep_double("fch1", inf.freq)
    hdr += prep_int("nbits", 32) # ??
    hdr += prep_double("tstart", inf.mjd_i + inf.mjd_f)
    hdr += prep_double("tsamp", inf.dt)
    hdr += prep_double("refdm", inf.dm)
    hdr += prep_string("HEADER_END")
    return hdr

if __name__ == "__main__":
    if len(sys.argv)==1:
        print "\nusage:  dat2tim.py file.dat\n"
        sys.exit()
    basefilenm = sys.argv[1].rstrip(".dat")
    inf = presto.read_inffile(basefilenm)
    outfile = open(basefilenm+".tim", "wb")
    outfile.write(infodata_to_sigproc_header(inf))
    outfile.close()
    # Do this the simple way
    #os.sys("cat %s >> %s"%())