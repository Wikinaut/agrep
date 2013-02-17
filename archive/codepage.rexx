/* T.Gries 12.08.1996 */

how_many_per_line=2
TAB='09'x

/* 
	Example:
	
   	CODEPAGE 850 > 850.LST


   This program generates a look-up table for all 256 characters of the
   selected codepage to faciliate the manual editing of two correspondence
   tables.
   
   character input range [00..FF] --> low1 --> low2
   
   The characters of the low1-table are the nearest lower-case characters
   in the selected codepage.
   
   The characters of the low2-table are mapped to lower-case 7-bit ASCII;
   these low2 characters do not contain accented characters for example.
   
   All non-characters (e.g. graphical characters and symbols) will be
   preserved; they will not be changed.
   
*/
   
   
parse arg codepage

if strip(codepage)='' then do
	say           'Please enter the codepage number (e.g. 850)'
	call charout ,'or press <enter> to use the current codepage: '
	codepage= linein()
end

if strip(codepage)<>'' then 'CHCP' codepage

lower_ASCII	= 'abcdefghijklmnopqrstuvwxyz'
upper_ASCII	= 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'

/* CODEPAGE 850 */

/* The 128 characters above 7Fh and two possible mappings to "lowercase" */

upper_80_BF ='€‚ƒ„…†‡ˆ‰Š‹Œ‘’“”•–—˜™š›œŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿'
low1_80_BF  ='‡‚ƒ„…†‡ˆ‰Š‹Œ„†‚‘‘“”•–—˜”›œŸ ¡¢£¤¤¦§¨©ª«¬­®¯°±²³´ ƒ…¸¹º»¼½¾¿'
low2_80_BF  ='cueaaaaceeeiiiaaeaaooouuyouoboxfaiounnao?r#24!<>#####aaac####cy#'

upper_C0_FF ='ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ'
low1_C0_FF  ='ÀÁÂÃÄÅÆÆÈÉÊËÌÍÎÏĞĞˆ‰ŠÕ¡Œ‹ÙÚÛÜİß¢á“•ääæçè£–—ììîïğñòóôõö÷øùúûüışÿ'
low2_C0_FF  ='ÀÁÂÃÄÅaaÈÉÊËÌÍÎoddeeeiiiiÙÚÛÜİißosoooomppuuuyyîïğñò3põö÷øùú132şÿ'

/* CODEPAGE 437 */

/* The 128 characters above 7Fh and two possible mappings to "lowercase" */

upper_80_BF ='€‚ƒ„…†‡ˆ‰Š‹Œ‘’“”•–—˜™š›œŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿'
low1_80_BF  ='‡‚ƒ„…†‡ˆ‰Š‹Œ„†‚‘‘“”•–—˜”›œŸ ¡¢£¤¤¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿'
low2_80_BF  ='cueaaaaceeeiiiaaeaaooouuyoucbypfaiounnao?©ª24!<>###³´µ¶·¸¹º»¼½¾¿'

upper_C0_FF ='ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ'
low1_C0_FF  ='ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ'
low2_C0_FF  ='ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßabgpssmtptddopeïğ+><ôõö÷øùúûü2şÿ'


z=''
zz=''

do iu=0 to 255
  
  zu=d2c(iu)
    
  zl1= translate(zu,lower_ASCII||low1_80_BF||low1_C0_FF,upper_ASCII||upper_80_BF||upper_C0_FF,'#')
  zl2= translate(zu,lower_ASCII||low2_80_BF||low2_C0_FF,upper_ASCII||upper_80_BF||upper_C0_FF,'#')

  /* say zu zl1 zl2 */
  
  zz=zz||d2c(iu)
	
  hxu=d2x(iu) ; if length(hxu)=1 then hxu='0'hxu
  hxl1=d2x(c2d(zl1)) ; if length(hxl1)=1 then hxl1='0'hxl1
  hxl2=d2x(c2d(zl2)) ; if length(hxl2)=1 then hxl2='0'hxl2
  
  z= z' {/*0x'hxu'*/,0x'hxl1',0x'hxl2',0x00, 0,0} /* 'spc(zu)||spc(zl1)||spc(zl2)' */,'

  if  iu//how_many_per_line == how_many_per_line-1 then do
  	if iu=255 then z=strip(z,'T',',')
  	say TAB||strip(z)
	z=''
  end


end

say
say
say '/* The 128 characters above 7Fh of codepage 'codepage': */'
say
say '/*' substr(zz,128+1,64) '*/'
say '/*' substr(zz,128+1+64) '*/'

return


spc:
/* print a dot in place of control characters below 0x20: */
parse arg xx
if xx < ' ' then cxx='.'
else cxx=xx
return cxx
