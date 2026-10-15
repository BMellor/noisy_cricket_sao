% design_lowpass.m - NoisyCricket ambient-level low-pass (MATLAB / Octave, no toolboxes)
%
% 4th-order Butterworth low-pass (prewarped bilinear), split into two biquads with
% b = b0 * [1 2 1] and unity DC gain each, quantised to Q14 for mic.c.
% Prints the LPx_* defines and the response of the quantised filter.

clear; clc;

fs = 23437.5;   % mic output rate
fc = 2500;      % -3 dB cutoff (Hz); the chirp band starts at 3250 Hz
Q  = 14;

W  = 2*fs*tan(pi*fc/fs);
sp = W * exp(1i*pi*[5 7]/8);          % upper-half-plane analogue poles

q = zeros(2, 3);
for k = 1:2
  zp = (1 + sp(k)/(2*fs)) / (1 - sp(k)/(2*fs));
  a  = real(poly([zp, conj(zp)]));
  b0 = sum(a) / 4;
  q(k, :) = round([b0, a(2), a(3)] * 2^Q);
  fprintf('#define LP%d_B0 %d\n#define LP%d_A1 %d\n#define LP%d_A2 %d\n', k, q(k,1), k, q(k,2), k, q(k,3));
end

f = [500 1000 2000 2500 3000 3250 3500 4000 4500 4750 5000 8000];
z = exp(-1i*2*pi*f/fs);
H = ones(size(f));
for k = 1:2
  b = q(k,1) * [1 2 1] / 2^Q;
  a = [1, q(k,2) / 2^Q, q(k,3) / 2^Q];
  H = H .* (b(1) + b(2)*z + b(3)*z.^2) ./ (a(1) + a(2)*z + a(3)*z.^2);
end
fprintf('\n%6s %7s\n', 'Hz', 'dB');
fprintf('%6d %7.1f\n', [f; 20*log10(abs(H))]);
