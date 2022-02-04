function load_sav, name
  
  ;Read all filter files saved in three polarization angle images

  restore,name, /VERBOSE

  ;A1 for 0 degree, B1 for 60 degree, C1 for 120 degree
   
  data = [ [A1moon3990],[B1moon3990],[C1moon3990],$
           [A1moon4249],[B1moon4249],[C1moon4249],$
           [A1moon3934],[B1moon3934],[C1moon3934],$
           [A1moon4025],[B1moon4025],[C1moon4025]]

  return, reform(data,[444,444,3,4])
end

function calc_aop, data

  common pols, p0, p60, p120

  upper = 2.*data[*,*,p0] - data[*,*,p60] - data[*,*,p120]
  lower = sqrt(3)*(data[*,*,p60] - data[*,*,p120])

  return, 0.5*atan(lower,upper)/!dtor
end

function calc_pb, data

  common pols, p0, p60, p120

  sq=(data[*,*,p0] + data[*,*,p60] + data[*,*,p120])^2d

  mul=-3d*[(data[*,*,p120]*data[*,*,p0])+(data[*,*,p120]*data[*,*,p60])+(data[*,*,p0]*data[*,*,p60])]

  return, (4d/3d)*sqrt(sq+mul)
end

function calc_tb, data

  common pols, p0, p60, p120

  return, 2d/3d*(data[*,*,p120] + data[*,*,p0] + data[*,*,p60])
end

pro draw_fig8
  common pols, p0, p60, p120

  w3990 = 0
  w4249 = 1
  w3934 = 2
  w4025 = 3

  p0   = 0
  p60  = 1
  p120 = 2


  ;Setting figure's parameters

  !p.thick=2.5
  !x.thick=2.5
  !y.thick=2.5
  !p.charsize=1.2
  !x.charsize=1.2
  !y.charsize=1.2
  !p.charthick=2.5

  set_plot,'Z'
  device,set_pixel_depth=24, decomposed=0

  ;Define observation times for all filter

  
  data = load_sav('draw_fig8.sav')


  ;Calculate angle of linear polarization (aP)

  aop3990 = calc_aop(data[*,*,*,w3990])
  aop4249 = calc_aop(data[*,*,*,w4249])
  aop3934 = calc_aop(data[*,*,*,w3934])
  aop4025 = calc_aop(data[*,*,*,w4025])
  

  ;Estimate size of 1 Rs on image

  x=[149+74,225+74,83+74]
  y=[222+74,143+74,178+74]
  cir_3pnt,x,y,r,x0,y0

  ;Define image center

  x0=444-224.85
  y0=444-(221.72+10)

  ;Calcuate aP within 0.3 Rs
  
  virtualimage=fltarr(444,444)+1.0
  
  for xpos=0,443 do begin
    for ypos=0,443 do begin
      rradius=sqrt((xpos-x0)^2+(ypos-y0)^2)
      if (rradius ge r*0.3) then begin
        virtualimage(xpos,ypos)=0.
      endif
    endfor
  endfor
  
  aa=where(virtualimage gt 0,count)
  
  print,'Mean aP within 0.3 Rs'
  print,'399.0 nm -> ', mean(aop3990(aa))
  print,'424.9 nm -> ', mean(aop4249(aa))
  print,'393.4 nm -> ', mean(aop3934(aa))
  print,'402.5 nm -> ', mean(aop4025(aa))


 
  ;Calcuate polarized brightness (pB)

  pb3990 = calc_pb(data[*,*,*,w3990])
  pb4249 = calc_pb(data[*,*,*,w4249])
  pb3934 = calc_pb(data[*,*,*,w3934])
  pb4025 = calc_pb(data[*,*,*,w4025])

  ;Calcuate total brightness (tB)

  tb3990 = calc_tb(data[*,*,*,w3990])
  tb4249 = calc_tb(data[*,*,*,w4249])
  tb3934 = calc_tb(data[*,*,*,w3934])
  tb4025 = calc_tb(data[*,*,*,w4025])


  ;Calcuate degree of linear polarization (dP)
  
  dp3990 = pb3990/tb3990
  dp4249 = pb4249/tb4249
  dp3934 = pb3934/tb3934
  dp4025 = pb4025/tb4025
 
 
  print,'Mean dP within 0.3 Rs'
  print,'399.0 nm -> ', mean(dp3990(aa))
  print,'424.9 nm -> ', mean(dp4249(aa))
  print,'393.4 nm -> ', mean(dp3934(aa))
  print,'402.5 nm -> ', mean(dp4025(aa))

  xsize=444

  ;Draw figure8

  cgDisplay, 444*4, 444*4+130
  cgErase
 
  ;Draw pB,tB,dP,aP

  loadct,33

  cgColorbar,ncolors=255,position=[0.02,0.97,0.23,0.99],title='pB (DN/s)',range=[0,40000],charthick=1.5,color='black',textthick=1.5,charsize=1.6;x0,y0,x1,y1

  tvscl,bytscl(pb3990,min=0,max=40000),xsize*0, xsize*3
  tvscl,bytscl(pb4249,min=0,max=40000),xsize*0, xsize*2
  tvscl,bytscl(pb3934,min=0,max=40000),xsize*0, xsize*1
  tvscl,bytscl(pb4025,min=0,max=40000),xsize*0, xsize*0

  cgColorbar,ncolors=255,position=[0.27,0.97,0.48,0.99],title='tB (DN/s)',range=[0,100000],charthick=1.5,color='black',textthick=1.5,charsize=1.6;x0,y0,x1,y1
  
  tvscl,bytscl(tb3990,min=0,max=100000),xsize*1, xsize*3
  tvscl,bytscl(tb4249,min=0,max=100000),xsize*1, xsize*2
  tvscl,bytscl(tb3934,min=0,max=100000),xsize*1, xsize*1
  tvscl,bytscl(tb4025,min=0,max=100000),xsize*1, xsize*0

  cgColorbar,ncolors=255,position=[0.52,0.97,0.73,0.99],title='dP',range=[0,0.5],charthick=1.5,color='black',textthick=1.5,charsize=1.6;x0,y0,x1,y1
  
  tvscl,bytscl(dp3990,min=0,max=0.5),xsize*2, xsize*3
  tvscl,bytscl(dp4249,min=0,max=0.5),xsize*2, xsize*2
  tvscl,bytscl(dp3934,min=0,max=0.5),xsize*2, xsize*1
  tvscl,bytscl(dp4025,min=0,max=0.5),xsize*2, xsize*0

  loadct,34

  cgColorbar,ncolors=255,position=[0.77,0.97,0.98,0.99],title='aP',range=[-90,90],charthick=1.5,color='black',textthick=1.5,charsize=1.6,tickinterval=30;x0,y0,x1,y1

  tvscl,bytscl(aop3990,min=-90.0,max=90.0),xsize*3, xsize*3
  tvscl,bytscl(aop4249,min=-90.0,max=90.0),xsize*3, xsize*2
  tvscl,bytscl(aop3934,min=-90.0,max=90.0),xsize*3, xsize*1
  tvscl,bytscl(aop4025,min=-90.0,max=90.0),xsize*3, xsize*0

  ; Draw captions

  loadct, 0
 
  name = ['pB', 'tB', 'dP', 'aP']
  wave = ['3990','4249','3934','4025']
  time = [ ['17:35:44 UT','17:35:58 UT'],$
           ['17:36:00 UT','17:36:15 UT'],$
           ['17:36:16 UT','17:36:31 UT'],$
           ['17:36:33 UT','17:36:47 UT'] ]
  time = reform(time,[2,4])
  
  for y=0, 3 do begin
    for x=0, 3 do begin
      xyouts,0.02 + 0.25*x, 0.71 - 0.232*y, time[0,y] +' !20&!n!x '+ time[1,y],color=255,charsize=2.0,charthick=2.7,/normal
      xyouts,0.06 + 0.25*x, 0.91 - 0.232*y, 'DICE ' + wave[y] + ' !6!sA!r!u!9 %!6!n!x ' + name[x], color=255,charsize=2.0,charthick=2.7,/normal
      tvcircle,r,x0 + xsize*x, y0 + xsize*y, color=255,thick=5
      if x gt 1 then tvcircle,r*0.3,x0 + xsize*x, y0 + xsize*y, color=150,thick=5
    endfor
  endfor


  ;Make figure 8 in png file

  fullimage=tvrd(/true)
  write_png, 'figure8.png',fullimage

end
