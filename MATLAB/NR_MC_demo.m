% Randall Ali = randall.ali@esat.kuleuven.be
% Created:           Oct 29, 2018
% Last update:       Nov 2, 2019
% For SPAI course 2019 - A framework for carrying out noise reduction in
% the STFT domain
%%
% Code needs auxiliry functions provided by randall to run
%% Setup Configuration
clear all;
clc;
addpath(genpath(fullfile('..')));   % add the previous folders 

% combine signals into one matrix and truncate the longer signal
[y_TD(:,1) fs] = audioread('part3a_pd2.wav');
inputSize = length(y_TD);
[z_TD fs] = audioread('part3a_pd1.wav');
inputSize = min(length(y_TD),length(z_TD));
y_TD(inputSize:length(y_TD)-1,:) = [];
y_TD(:,2) = z_TD(1:inputSize,1);

M = size(y_TD,2); % number of microphones
eref = [1; 0;];
eref = zeros(M,1);
eref(1) = 1;

%% Transform to STFT domain
N_fft = 512;                        % number of FFT points
R_fft = N_fft/2;                    % shifting (50% overlap)
win = sqrt(hann(N_fft,'periodic')); % analysis window
N_half = floor(N_fft/2)+1;          % number of bins in onsided FFT 
freqs = 0:fs/N_fft:fs/2;            % one-sided set of actual frequencies

% compute the STFT (frequency x frame x channel) and plot
y_STFT = calc_STFT(y_TD, fs, win, N_fft, N_fft/R_fft, 'onesided'); % noisy microphone signals
[N_freqs, N_frames] = size(y_STFT(:,:,1)); % total number of freq bins + time frames  
figure; imagesc(1:N_frames, freqs, mag2db(abs(y_STFT(:,:,1))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w'), xlabel('Time Frames'), ylabel('Frequency (Hz)'), title('Spectrogram of noisy speech'); 

% Compute the Speech Presence Probability on the first mic (reference)
[noisePowMat, SPP] = spp_calc(y_TD(:,1),N_fft,R_fft);
figure; imagesc(1:N_frames, freqs,SPP); colorbar; axis xy; set(gcf,'color','w');  set(gca,'Fontsize',14), xlabel('Time Frames'), ylabel('Frequency (Hz)'), title('Speech Presence Probability for ref mic');
%% Define necessary constants
Rnn = cell(N_freqs,N_frames);  Rnn(:) = {zeros(M,M)};      % Noise Only (NO) corr. matrix. Initialize to zeros
Ryy = cell(N_freqs,N_frames);  Ryy(:) = {zeros(M,M)};      % Speech + Noise (SPN) corr. matrix. Initialize to zeros

lambda = 0.77;                                              % Forgetting factor for computing correlation matrices - change values to observe effects on results
SPP_thr = 0.5;                                             % Threshold for the SPP - also change values to observe effects on results

C_mc_stft = zeros(N_freqs,N_frames);                       % output speech estimate cepstral smoothing
M_mc_stft = zeros(N_freqs,N_frames);                       % output speech estimate MVDR with CS RTF
X_mc_stft = zeros(N_freqs,N_frames);                       % output speech estimate MVDR with CW RTF
S_mc_stft = zeros(N_freqs,N_frames);                       % output speech estimate MWF
G_mc_stft = zeros(N_freqs,N_frames);                       % single channel filter gains

W_mc_CW = (1/M)*ones(M,N_freqs);                           % MVDR filter with CW RTF
W_mc_CS = (1/M)*ones(M,N_freqs);                           % MVDR filter with CS RTF

h_cs = zeros(M, N_freqs, N_frames);                        % estimated RTF with CS
h_cw = zeros(M, N_freqs);                                  % estimated RTF with CW
%%
sigma_s = zeros(N_freqs,N_frames);            %single channel speech estimate
sigma_n = zeros(N_freqs,N_frames);            %single channel moise estimate

alpha_n = 0.72; alpha_s=0.999;                  %single channel forgetting factors
Xi_min = 1e-8;                                
%% Filtering
tic
for l=2:N_frames % Time index - start from 2 since things would involve "l-1" - this is just a convenience for the first iteration 
    %Multichannel
    for k = 1:N_freqs % Freq index
        y_mc (:, 1)= y_STFT(k,l,:); %get current frame from all channels
        
        if(SPP(k,l)<SPP_thr | l<3) 
            Rnn{k,l} = lambda*Rnn{k, l-1}+(1-lambda)*(y_mc*y_mc');
            Ryy{k,l} = Ryy{k,l-1};
        else
            Rnn{k,l} = Rnn {k,l-1};
            Ryy{k,l} = lambda*Ryy{k, l-1}+(1-lambda)*(y_mc*y_mc');
        end
     
        %Estimate RTF using Covariance Subtraction
        Rxx = Ryy{k,l}-Rnn{k,l};                        %get speech only correlation matrix
        [V, D] = eig(Rxx);                              %perform eigenvalue decomposition on it
        [d, ind]= sort(diag(D));                        %sort eigenvalues descendingly
        Vs = V(:, ind);                                 %sort eigenvectors based on value sorting
        Vprin = Vs(:,M);                                %select last column corresponding to principal eigen vector
        h_cs(:,k) = Vprin/(eref.'*Vprin);               %compute RTF estimate

        %Covriance subtraction with no EVD
%         Rxx = Ryy{k,l}-Rnn{k,l}; %get speech only correlation matrix
%         h_cs(:,k) = (Rxx*eref)/(eref.'*Rxx*eref); %compute RTF estimate
        
        %Estimate RTF using Covriance Whitening
%         [U, D]=eig(Ryy{k,l}, Rnn{k,l});
%         [d, ind]= sort(diag(D));
%         Us = U(:, ind);
%         Q = inv(U');
%         Qprin = Q(:,M);
%         h_cw(:,k) = Qprin/(eref.'*Qprin);
        
        %Calculate filters
        W_mc_CS(:,k) = (pinv(Rnn{k,l})*h_cs(:,k))/((h_cs(:,k)'*pinv(Rnn{k,l}))*h_cs(:,k)); %filter mvdr with RTF estimate (CS) 
%       W_mc_CW(:,k) = (pinv(Rnn{k,l})*h_cw(:,k))/(h_cw(:,k)'*pinv(Rnn{k,l})*h_cw(:,k)); %filter mvdr with RTF estimate (CS) 
        
        %Apply filters to signal
        M_mc_stft(k,l) = W_mc_CS(:,k)'*y_mc;             %output MVDR with CS RTF    
%       X_mc_stft(k,l) = W_mc_CW(:,k)'*y_mc;             %output MVDR with CW RTF 
    end     
end

%single channel 
for l=2:N_frames
  for k = 1:N_freqs % Freq index       
        if(SPP(k,l) < SPP_thr ) % assume noise only
            sigma_n(k,l) =  alpha_n*sigma_n(k,l-1) + (1-alpha_n)*(abs(M_mc_stft(k,l))^2);
        else 
            sigma_n(k,l) = sigma_n(k,l-1);
        end
        
        sigma_s(k,l) = max(alpha_s*(abs(S_mc_stft(k,l-1))^2) + (1-alpha_s)*(abs(M_mc_stft(k,l))^2-sigma_n(k,l)), Xi_min*sigma_n(k,l));
        G_mc_stft(k,l) = sigma_s(k,l)/(sigma_s(k,l)+sigma_n(k,l));
        S_mc_stft(k,l) = G_mc_stft(k,l)*M_mc_stft(k,l);  
  end
end
%% Temporal cepstrum smoothing of gain
% Transform gain to cepstral domain
Cep = zeros(N_freqs, N_frames);
for l=1:N_frames
        x = log(G_mc_stft(:,l));
        Cep(:,l) = ifft(x);
end

% Look for pitch frequency in the range fs/f0_high to fs/f0_low
kpitch = zeros(N_frames, 1);
f0_low = 180; f0_high = 500;
for l=2:N_frames 
    max = -100;
    for k= floor(fs/f0_high):floor(fs/f0_low)
            if(Cep(k,l)>max)
             max = Cep(k,l);
             kpitch(l) = k;
            end
    end 
end

% Smooth the spectral gains(No smoothing for k<5, beta_pitch for k belonging to K, beta for remaining values of k)
Cep_smooth = zeros(N_freqs, N_frames);
beta_pitch =0.2;
beta = 0.62;
for l=2:N_frames 
    K = [ kpitch(l)-1, kpitch(l), kpitch(l)+1];
    for k=1:N_freqs
        if(k<5)
            Cep_smooth(k,l) = Cep(k,l);
        elseif(ismember(k,K))
            Cep_smooth(k,l) = beta_pitch*Cep_smooth(k,l-1) + (1-beta_pitch)*Cep(k,l);
        else
            Cep_smooth(k,l) = beta*Cep_smooth(k,l-1) + (1-beta)*Cep(k,l);
        end
    end 
end

% Transform back to frequency domain and get filtered signal
G_smooth = zeros(N_freqs, N_frames);
for l=1:N_frames % Time index - start from 2 since things would involve "l-1" - this is just a convenience for the first iteration
       G_smooth(:,l) = exp(fft(Cep_smooth(:,l)));
    for k=1:N_freqs
       if(G_smooth(k,l)>1) G_smooth(k,l) = 1; end
       C_mc_stft(k,l) = G_smooth(k,l)*M_mc_stft(k,l);
    end
    
end
toc
%% Tansform back to time domain
% PLOT YOUR ENHANCED STFTs
times_stft = ((1:N_frames));
figure; imagesc(times_stft,freqs,mag2db(abs(y_STFT(:,:,1))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w');set(gca,'Fontsize',14); xlabel('Time (s)'), ylabel('Frequency (Hz)'), title('microphne signal, 1st mic');
figure; imagesc(times_stft,freqs,mag2db(abs(S_mc_stft(:,:))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w'); set(gca,'Fontsize',14);xlabel('Time (s)'), ylabel('Frequency (Hz)'),title('MWF');
figure; imagesc(times_stft,freqs,mag2db(abs(M_mc_stft(:,:))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w'); set(gca,'Fontsize',14);xlabel('Time (s)'), ylabel('Frequency (Hz)'),title('Multi Channel Enhancement CS');
figure; imagesc(times_stft,freqs,mag2db(abs(C_mc_stft(:,:))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w'); set(gca,'Fontsize',14);xlabel('Time (s)'), ylabel('Frequency (Hz)'),title('Cepstral smoothing output');
%figure; imagesc(times_stft,freqs,mag2db(abs(X_mc_stft(:,:))), [-65, 10]); colorbar; axis xy; set(gcf,'color','w'); set(gca,'Fontsize',14);xlabel('Time (s)'), ylabel('Frequency (Hz)'),title('Multi Channel Enhancement CW');

% compute the iSTFT - convert back to time domain (plot to also have a look)
time = 0:1/fs:(length(y_TD(:,1))-1)/fs;
s_mc_TD = calc_ISTFT(S_mc_stft, win, N_fft, N_fft/R_fft, 'onesided'); %MWF output based on MVDR
m_mc_TD = calc_ISTFT(M_mc_stft, win, N_fft, N_fft/R_fft, 'onesided'); %MVDR output CS
% x_mc_TD = calc_ISTFT(X_mc_stft, win, N_fft, N_fft/R_fft, 'onesided'); %MVDR output CW
c_mc_TD = calc_ISTFT(C_mc_stft, win, N_fft,N_fft/R_fft,'onesided');% Cepstral smoothing output

% Write audio files 
audiowrite('../audio_processed/enhanced_MC.wav',s_mc_TD(:,1),fs);
%% Play signal                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       _mc_TD,fs); 
%sound(y_TD(:,1), fs); %play input signal (ref mic)
sound(s_mc_TD, fs)










