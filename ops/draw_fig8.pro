pro draw_fig8

  dir=''

  ;Setting figure's parameters

  !p.thick=2.5
  !x.thick=2.5
  !y.thick=2.5
  !p.charsize=1.5
  !x.charsize=1.5
  !y.charsize=1.5
  !p.charthick=2.5

  device,decomposed=0

  ;Read 4025 filter files saved in three polarization angle images

  restore,dir+'draw_fig8.sav', /VERBOSE

  ;A1moon for 0 degree, B1moon for 60 degree, C1moon for 120 degree


  ;Define observation times for 4025 filter

  timearray=['17:36:33 UT','17:36:47 UT']


  ;Calculate angle of linear polarization (aP)

  uppermoon=2.*(A1moon)-B1moon-C1moon
  lowermoon=sqrt(3)*(B1moon-C1moon)

  thetamoon=0.5*atan(lowermoon,uppermoon)/!dtor


  ;Draw aP image

  ct_rainbow,/load
  window,2,xs=444,ys=444

  tvscl,bytscl(thetamoon,min=-90.0,max=90.0)
  ct_rainbow,/load
  ;cgcolorbar,range=[-90,90],Divisions=4,charsize=1.5,charthick=3.0,color='white',Position = [0.12, 0.85, 0.88, 0.9]
  loadct,0
  xyouts,0.12,0.93,'DICE 402.5 nm pB Angle',color=255,charsize=2.0,charthick=2.7,/normal
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal

  ;Estimate size of 1 Rs on image

  x=[149+74,225+74,83+74]
  y=[222+74,143+74,178+74]
  cir_3pnt,x,y,r,x0,y0

  ;Define image center

  x0=444-224.85
  y0=444-(221.72+10)


  ;Draw circles of 1Rs and 0.3 Rs

  tvcircle,r,x0,y0,color=255,thick=5

  tvcircle,r*0.3,x0,y0,color=150,thick=5

  ;Calcuate aP within 0.3 Rs

;  virtualimage=fltarr(444,444)+1.0
;
;  for xpos=0,443 do begin
;    for ypos=0,443 do begin
;      rradius=sqrt((xpos-x0)^2+(ypos-y0)^2)
;      if (rradius ge r*0.3) then begin
;        thetamoon(xpos,ypos)=0.
;        virtualimage(xpos,ypos)=0.
;      endif
;    endfor
;  endfor
;
;  aa=where(virtualimage gt 0,count)
;
;  print,mean(thetamoon(aa))
;  print,median(thetamoon(aa))
;  print,stddev(thetamoon(aa))
;
; xyouts,0.02,0.10,'Mean = 69.2, Median = 75.4, Std = 32.6',color=255,charsize=1.5
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal


  ;Save figures

  fullimage=tvrd(/true)
  write_png,dir+'4025apmoon.png',fullimage



  ;Calcuate polarized brightness

  square4025moon=(A1moon+B1moon+C1moon)^2d

  multiplies4025moon=-3d*[(C1moon*A1moon)+(C1moon*B1moon)+(A1moon*B1moon)]

  rootsquare4025moon=sqrt(square4025moon+multiplies4025moon)

  pb4025moon=(4d/3d)*rootsquare4025moon

  ;Calcuate total brightness (tB)

  tb4025moon=2d/3d*(C1moon+A1moon+B1moon)


  ;Draw pB images

  loadct,33

  window,6,xs=444,ys=444

  tvscl,bytscl(pb4025moon,min=0,max=40000)
  loadct,33
  ;cgcolorbar,range=[0,40000],Divisions=4,charsize=1.0,charthick=3.0,color='white',Position = [0.12, 0.85, 0.88, 0.9]
  loadct,0
  xyouts,0.22,0.93,'DICE 402.5 nm pB',color=255,charsize=2.0,charthick=2.7,/normal
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal


  ;Draw circles of 1Rs

  x0=444-224.85
  y0=444-(221.72+10)

  tvcircle,r,x0,y0,color=255,thick=5

  ;Save pB figure

  fullimage=tvrd(/true)
  write_png,dir+'4025pbmoon.png',fullimage


  ;Draw tB image

  loadct,33

  window,10,xs=444,ys=444

  tvscl,bytscl(tb4025moon,min=0,max=100000)

  loadct,33
  ;cgcolorbar,range=[0,100000],Divisions=4,charsize=1.0,charthick=3.0,color='white',Position = [0.12, 0.85, 0.88, 0.9]
  loadct,0
  xyouts,0.22,0.93,'DICE 402.5 nm tB',color=255,charsize=2.0,charthick=2.7,/normal
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal


  ;Draw circles of 1Rs

  x0=444-224.85
  y0=444-(221.72+10)

  tvcircle,r,x0,y0,color=255,thick=5

  ;Save tB figure

  fullimage=tvrd(/true)
  write_png,dir+'4025tbmoon.png',fullimage


  ;Calcuate degree of linear polarization (dP)

  pbdegree4025moon=pb4025moon/tb4025moon


  ;Draw dP image

  loadct,33

  window,14,xs=444,ys=444

  tvscl,bytscl(pbdegree4025moon,min=0,max=0.5)

  loadct,33
  ;cgcolorbar,range=[0,0.5],Divisions=4,charsize=1.0,charthick=3.0,color='white',Position = [0.12, 0.85, 0.88, 0.9]
  loadct,0
  xyouts,0.12,0.93,'DICE 402.5 nm pB degree',color=255,charsize=2.0,charthick=2.7,/normal
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal


  ;Draw circles of 1Rs and 0.3 Rs


  x0=444-224.85
  y0=444-(221.72+10)

  tvcircle,r,x0,y0,color=255,thick=5

  tvcircle,r*0.3,x0,y0,color=150,thick=5



;  ;Calcuate dP within 0.3 Rs
;
;  virtualimage=fltarr(444,444)+1.0
;
;  for xpos=0,443 do begin
;    for ypos=0,443 do begin
;      rradius=sqrt((xpos-x0)^2+(ypos-y0)^2)
;      if (rradius ge r*0.3) then begin
;        virtualimage(xpos,ypos)=0.
;      endif
;    endfor
;  endfor
;
;  aa=where(virtualimage gt 0,count)
;
;  print,mean(pbdegree4025moon(aa))
;  print,median(pbdegree4025moon(aa))
;  print,stddev(pbdegree4025moon(aa))

  ;xyouts,0.02,0.10,'Mean=0.099, Median=0.100, Std=0.017',color=255,charsize=1.5
  xyouts,0.02,0.02,timearray(0)+' !20&!n!x '+timearray(1),color=255,charsize=2.0,charthick=2.7,/normal

  ;Save dP image

  fullimage=tvrd(/true)
  write_png,dir+'figure8.png',fullimage

  stop

end
