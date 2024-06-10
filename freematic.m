T = readtable("1.csv");
Tfiltered = T;

plot(T.ts,T.accy);
hold on;


function ret = fillWithZero(n) 
    n(isnan(n)) = 0;
    ret = n;
end


function ret = fillWithFirst(n) 
    f = n(find(~isnan(n), 1 ));
    n(isnan(n)) = f;
    ret = n;
end

function ret = getColors(n, minv, maxv)
    cd = colormap(jet(512));
    it = interp1(linspace(minv, maxv, length(cd)), cd, n);
    cd = uint8(it.'*255); 
    cd(4,:) = 255;
    ret = cd;
end

j
Tfiltered.lat = fillWithFirst(Tfiltered.lat);
Tfiltered.long = fillWithFirst(Tfiltered.long);
Tfiltered.kmh = fillWithZero(Tfiltered.kmh);
Tfiltered.accx = fillWithZero(Tfiltered.accx);

ts = downsample(Tfiltered.ts,10);
lat = downsample(Tfiltered.lat,10);
long = downsample(Tfiltered.long, 10);
kmh = downsample(Tfiltered.kmh, 10);
accx = downsample(abs(Tfiltered.accx), 10);



colorbar;
caxis([0, 10]);
mapcolors = getColors(accx, 0, 10);

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

