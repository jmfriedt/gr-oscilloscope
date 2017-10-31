pkg load signal  % matlab users: remove octave commands
clear all;close all
more off
fcenter=500e6 % 500 MHz center freq
fcenter=473e6 % 500 MHz center freq
fs=200e6      % sampling frequency after transposition and decimation
fs=2.3e6      % sampling frequency after transposition and decimation
vmax=1000/3.6 % plane at 1000 km/h
fstep=5       % freq steps
nbp=4096*32
range_res=3e8/2/fs
range=nbp*range_res
doppler_res=fs/nbp

fd=2*vmax/3e8*fcenter;
ref=read_complex_binary('/tmp/ref.bin',fs/10);
mes=read_complex_binary('/tmp/mes.bin',fs/10);
t=[0:1/fs:length(ref)/fs];t=t(1:end-1);

l=1;
for doppler=-fd:fstep:fd
  doppler
  lo=exp(j*2*pi*doppler*t)';
  signal=mes.*lo;
  % the range resolution is c/(2B), the max range is nbp*c/(2B)
  signalf=fft(signal-mean(signal),nbp);
  reff=fft(ref-mean(ref),nbp);
  solution(:,l)=fftshift(abs(ifft(reff.*conj(signalf),nbp)));
  l=l+1;
end
% imagesc([-fd:fstep:fd],[0:4095],solution)

