The CD-ROM is organised in two main folders, one for each of the two reference phantoms (AM: adult male, AF: adult female). 

Each folder contains the following files:
• The array of organ identification numbers (in ASCII format); the file names are: 
	AM.dat
	AF.dat
• A list of individually segmented structures, their identification numbers, and assigned media (Appendix A); the file names are: 
	AM_organs.dat
	AF_organs.dat
• A list of the media, their elemental compositions and densities (Appendix B); the file names are: 
	AM_media.dat
	AF_media.dat
• The mass ratios of bone constituents (trabecular bone, red and yellow bone marrow) in the spongiosa regions; the file names are: 
	AM_spongiosa.dat
	AF_spongiosa.dat
• The mass ratios of blood in various body tissues; the file names are: 
	AM_blood.dat
	AF_blood.dat

In the organ ID arrays, the organ IDs are listed slice by slice, within each slice row by row, within each row column by column. That means, the column index changes fastest, then the row index, then the slice index. The numbers of columns, rows and slices (i.e., the array dimensions) of both phantoms are given in Table 3 together with the voxel dimensions.

Slice numbers increase from the toes up to the vertex of the body; row numbers increase from front to back; and column numbers increase from right to left side.

There is skin with a different organ identification number at the top and bottom of the model (first and last slice). Thus, it can either be used or neglected (by assigning air or vacuum as medium) depending on the situation considered. If assigned skin as medium, it increases the body height as well as the total body mass.

The organ identification numbers are stored in portions of 16, but the number of columns is not a multiple of 16. Therefore, appropriate care has to be applied for reading the data. As an example, a FORTRAN programme that reads the data of the female reference computational phantom and stores them into a (299 x 137 x 348) array is given in the following:

      program readdata
*     Programme to read the voxel phantom data from ASCII file
*     AF.dat and store them in a three-dimensional array of
*     organ identification numbers
      dimension norgin(300),nodum(16),noid(300,200,350)
*     Number of columns, rows and slices:
      ncol=299
      nrow=137
      nsli=348
      open (10,file='AF.dat')
*     Read phantom file, assign organ identification numbers to 
*     array positions
*     The organ identification numbers are stored in portions of 16, 
*     but the number of columns is not a multiple of 16.
*     Therefore, when ncol OIDs are read, this ends right in the
*     middle of a line, and the rest is ignored. Therefore, the next
*     "read" statement has to account for this; the last line that
*     has been read only partially, has to be read again, whereby
*     that first part that has been registered already needs to be
*     skipped. Therefore, the number "ndum" of these items has to be
*     tracked.
      write (6,'('' Reading of phantom file started'')')
      nrorea=ncol/16
      ndifcol=ncol-16*nrorea
      ndum=0
      do 40 nsl=1,nsli
      do 40 nr=1,nrow
      if (ndum.ne.0) then
	  backspace (10)
          read (10,*) (nodum(i),i=1,ndum), (norgin(i),i=1,ncol)
      else
          read (10,*) (norgin(i),i=1,ncol)
      endif
      do 30 nc=1,ncol
      noid(nc,nr,nsl)=norgin(nc)
   30 continue
      ndum=ndum+ndifcol
      if (ndum.ge.16) ndum=ndum-16
   40 continue
      write (6,'(''   ... finished'')')
      close (10)
      end
 
