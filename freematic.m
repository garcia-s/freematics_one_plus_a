

clear;clc;close all
disp('INICIO')

%% LECTURA DE DATOS
[filename, pathname] = uigetfile('*.*');
filename = fullfile(pathname, filename); 
T = readtable(filename);
Tfiltered = T;


function ret = fillWithZero(n) 
    n(isnan(n)) = 0;
    ret = n;
end


function ret = fillWithFirst(n) 
    f = n(find(~isnan(n), 1 ));
    n(isnan(n)) = f;
        ret = ns
end

function ret = getColors(n, minv, maxv)
    cd = colormap(jet(512));
    it = interp1(linspace(minv, maxv, length(cd)), cd, n);
    cd = uint8(it.'*255); 
    cd(4,:) = 255;
    ret = cd;
end

Tfiltered.accy = Tfiltered.accy + 30
Tfiltered.lat = fillWithFirst(Tfiltered.lat);
Tfiltered.long = fillWithFirst(Tfiltered.long);
Tfiltered.kmh = fillWithZero(Tfiltered.kmh);
Tfiltered.accx = fillWithZero(Tfiltered.accx);
Tfiltered.accy = fillWithZero(Tfiltered.accy);

ts = downsample(Tfiltered.ts,20);
lat = downsample(Tfiltered.lat,20);
long = downsample(Tfiltered.long, 20);
kmh = downsample(Tfiltered.kmh, 20);
accx = downsample(abs(Tfiltered.accx), 20);
accy = downsample(Tfiltered.accy, 20);


figure;
plot(ts, kmh);
title("Tiempo vs Velocidad");

figure;
plot(ts, accy);

figure;
plot(ts, accx);

figure;
colorbar;
caxis([0, 10]);
mapcolors = getColors(accx, 0, 10);

gx = geoaxes;
h = plot(geoaxes, lat,long, 'LineWidth', 5);
drawnow;
set(h.Edge,'ColorBinding','interpolated','ColorData',mapcolors);


figure;
colorbar;
caxis([0, 10]);
mapcolors = getColors(kmh, 0, 10);

gx = geoaxes;
h = plot(geoaxes, lat,long, 'LineWidth', 5);
drawnow;
set(h.Edge,'ColorBinding','interpolated','ColorData',mapcolors);
%
% figure;
% subplot(2, 1, 1);
% plot(Tfiltered.accy);
% title('Original Signal');
%
% subplot(2, 1, 2);
% plot(denoised_signal);
% title('Denoised Signal');
% C = sgolayfilt(Tfiltered.kmh,  3, 7);
% plot(T.ts,B');
% hold on;
%
%
% plot(T.ts,C');
% hold on;

