
 %% open the bin file to work on it ( bitsream )
fid = fopen('RaceHorses_832x480_30_QP37.264', 'r');
data = fread(fid);

%% define the header as a delimiter 
cond=[0 0 0 1];

%% find indexes of the starting of each packet in the bin file ( bitsream )
k = strfind(data',cond);

%% find the size of each packet on the bin file ( bitsream )
Packets_size=diff(k);

%% verification 
% verif=[];
% 
% for i=1:length(k)
% verif=[verif data(k(i):k(i)+3,1)];
% end

%% define the starting index of the packet to be deleted
lostFrame = 15;
starting=k(lostFrame+5);

%% define the ending index of the packet to be deleted
ending=Packets_size(lostFrame+5);

%% create the new vector file after delting
newdata = data([1:starting, ending+starting:end]);

%% the size of the new file
size_newdata=size(newdata);
%% save the new binary file

fid2 = fopen('Altered_RaceHorses_832x480_30_QP37.264', 'w');
ind=fwrite(fid2,newdata);
