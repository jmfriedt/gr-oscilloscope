% 5 m cable and 2 m cable

meanvalue=(5000/5)/2; % ns
x=load('gnuradio310_xcorr_RS.txt');
k=find(x(:,1)==2);
m1=meanvalue-mean(x(k,2)/5)
std(x(k,2)/5)
subplot(211)
plot((x(k,2)-mean(x(k,2)))/5)

k=find(x(:,1)==1);
m2=meanvalue-mean(x(k,2)/5)
std(x(k,2)/5)
hold on
plot((x(k,2)-mean(x(k,2)))/5)
xlabel('sample index (a.u.)')
ylabel('delay (ns)')
xlim([0 800])
(m1-m2)*.2 % 0.2 m / ns
m1*.2
m2*.2
