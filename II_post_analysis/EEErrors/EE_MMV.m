function Table1 = EE_MMV(kprime_external,inputdir,ngridk_external,ngridkm_external)
% This function builds onthe original code TEST.m written by Lilia Maliar, Serguei Maliar and Fernando Valli (2008) to compute the Euler Equation Errors in 
% "Solving the incomplete markets model with aggregate uncertainty using the Krusell-Smith algorithm" from the special  JEDC issue edited by Den Haan, Judd and Juillard (2008)   

%% Initialization from Maliar Maliar Valli (2010)
% Preallocation for identifier to matlab functions: https://www.mathworks.com/matlabcentral/answers/380256-matlab-confusion-of-function-and-variable-names-after-load
alpha = 0;
beta = 0;
gamma = 0;
mu = 0;
file_path = [inputdir,'matlab/MMV/Solution_to_model_nKM',num2str(ngridkm_external),'-nk',num2str(ngridk_external),'.mat'];
load(file_path);
clearvars -except kprime_external inputdir nstates_ag ur_b ur_g a er_b er_g prob alpha l_bar k km k_min k_max mu delta B a2 epsilon2 epsilon_u gamma epsilon_e beta km_min km_max kss ngridk_external ngridkm_external
kprime = kprime_external;
clearvars kprime_external
%{
variables_to_keep = 'nstates_ag ur_b ur_g a er_b er_g prob alpha l_bar k km k_min k_max mu delta B a2 epsilon2 epsilon_u gamma epsilon_e beta';
variables_to_keep_cell = strsplit(variables_to_keep, ' ');
for i = 1:length(variables_to_keep_cell)
    eval([variables_to_keep_cell{i},'=S_.',variables_to_keep_cell{i},';']); % 'kprime=S_.kprime;'
end
eval(['clearvars -except kprime inputdir ',variables_to_keep]); %clearvars -except kprime inputdir nstates_ag ur_b ur_g a er_b er_g prob alpha l_bar k km k_min k_max mu delta B a2 epsilon2 epsilon_u gamma epsilon_e beta km_min km_max kss
%}
% Sanity checks
ngridk=ngridk_external;     % number of grid points on individual capital holdings
clearvars ngridk_external 
ngridkm=size(kprime,2);   % number of grid points for km 
nA = size(kprime,3);         % Aggregate shock's states
nepsilon = size(kprime,4); % Idiosyncratic employment status shock's states 

assert(ngridk == size(k,1));
assert(ngridkm == size(km,1));
assert(nA == nstates_ag);
assert(nA == 2);

clearvars ngridk ngridkm nA nepsilon

%% Loading Input for Test 
%{ 
Description: /DJJ/Inputs_for_test.mat: It contains three objects provided by Den Haan, Judd and Juillard, 2008): 
	1) "kdist" is the initial distribution of individual capital holding on a 1000-point grid for unemployed and 	employed agents; 
	2) "agshock" is a 10,000-period realization of the aggregate shock;
	3) "idshock" is a 10,000-period realization of the idiosyncratic shock for a single agent.
%}
file_path = [inputdir,'matlab/DJJ/Inputs_for_test.mat'];
load(file_path);

%% Euler Equation Errors Computation
%__________________________________________________________________________
simulation_type = 2;        % 1 STOCHASTIC, 2 'NON-STOCHASTIC'
N= 10000;
%__________________________________________________________________________
%
% 1. PARAMETERS
%__________________________________________________________________________

T=10000; % simulation length
J=1001;  % number of grid points for non-stochastic simulation (is equal to 
         % 1001 and not to 1000 because we include explicitly point k=0)
ur=zeros(nstates_ag,1); % vector of unemployment rates in two agg. states
ur(1)=ur_b; ur(2)=ur_g; 
%__________________________________________________________________________
%
% 2. SERIES
%__________________________________________________________________________

% Time series of aggregate variables

km_ag=zeros(T,1);       % aggregate capital (mean of capital distribution)
c_ag=zeros(T-1,1);      % aggregate consumption
income_ag=zeros(T-1,1); % aggregate income
wealth_ag=zeros(T-1,1); % aggregate wealth
i_ag=zeros(T-1,1);      % aggregate net investment

prod_ag=(agshock==1)*a(1)+(agshock==2)*a(2);   % aggregate productivity
labor_ag=(agshock==1)*er_b+(agshock==2)*er_g;  % aggregate employment (L in 
                                               % the paper)

irate=zeros(T-1,1); % interest rate
wage=zeros(T-1,1);  % wage

% Time series of individual variables

k_ind=zeros(T,1);        % individual capital
k_ind(1,1)=43;           % initial individual capital 
c_ind=zeros(T-1,1);      % individual consumption
income_ind=zeros(T-1,1); % individual income

% Beginning-of-period capital distributions

kdistu=zeros(T,J); % beginning-of-period capital distributions in all 
                   % periods for the unemployed
kdiste=zeros(T,J); % for the employed

% Initial-period capital distributions

% kdist has dimensionality J*2 with J=1001; in the last point J, the probability 
% is chosen to normalize the sum of probabilities to 1

kdistu(1,1:J)=[kdist(1:end-1,1)' 1-sum(kdist(1:end-1,1))]; % a raw vector of 
                       % the initial capital distribution for the unemployed
kdiste(1,1:J)=[kdist(1:end-1,2)' 1-sum(kdist(1:end-1,2))]; % for the employed

% Grid for the capital distribution

kvalues_min=0;   % minimum grid value
kvalues_max=100; % maximum grid value
kvalues=linspace(kvalues_min,kvalues_max,J)'; % grid for capital distribution
                                              % with J grid points
% _________________________________________________________________________
%
% 3. TRANSITION PROBABILITIES 
%__________________________________________________________________________

% prob_ag(i,j) is the probability of tomorrow's agg. shock (i=1,2) given 
% today's agg. shock (j=1,2)

prob_ag=zeros(2,2);
prob_ag(1,1)=prob(1,1)+prob(1,2); prob_ag(2,1)=1-prob_ag(1,1);  
prob_ag(2,2)=prob(3,3)+prob(3,4); prob_ag(1,2)=1-prob_ag(2,2);

% p_xy_zw is the probability of idiosyncratic shock epsilon'=w conditional 
% on aggregate shocks s'=y, s=x and idiosyncratic shock epsilon=z 

p_bb_uu = prob(1,1)/prob_ag(1,1); p_bb_ue=1-p_bb_uu;
p_bb_ee = prob(2,2)/prob_ag(1,1); p_bb_eu=1-p_bb_ee;
p_bg_uu = prob(1,3)/prob_ag(2,1); p_bg_ue=1-p_bg_uu;
p_bg_ee = prob(2,4)/prob_ag(2,1); p_bg_eu=1-p_bg_ee;
p_gb_uu = prob(3,1)/prob_ag(1,2); p_gb_ue=1-p_gb_uu;
p_gb_ee = prob(4,2)/prob_ag(1,2); p_gb_eu=1-p_gb_ee;
p_gg_uu = prob(3,3)/prob_ag(2,2); p_gg_ue=1-p_gg_uu;
p_gg_ee = prob(4,4)/prob_ag(2,2); p_gg_eu=1-p_gg_ee;

% Transition probabilities from one idiosyncratic shock, epsilon, to another, 
% epsilon', given that agg. shocks are s and s'

p_bb = [[p_bb_uu p_bb_ue]*ur_b; [p_bb_eu p_bb_ee]*er_b];
p_bg = [[p_bg_uu p_bg_ue]*ur_b; [p_bg_eu p_bg_ee]*er_b];
p_gb = [[p_gb_uu p_gb_ue]*ur_g; [p_gb_eu p_gb_ee]*er_g];
p_gg = [[p_gg_uu p_gg_ue]*ur_g; [p_gg_eu p_gg_ee]*er_g];


switch simulation_type
    case 2
%__________________________________________________________________________
%
% 4. NON-STOCHASTIC SIMULATION
%__________________________________________________________________________
for t=1:T
    
   % Aggregate capital
   
   km_ag(t)=kdistu(t,:)*kvalues*(1-labor_ag(t))+kdiste(t,:)*kvalues*labor_ag(t);
      % aggregate capital=capital of the unemployed + capital of the employed;
      % (1-labor_ag(t)) is the share of unemployed people in the economy
   
   % Prices
   
   irate(t)=alpha*prod_ag(t)*(km_ag(t)/labor_ag(t)/l_bar)^(alpha-1);
      % interest rate 
   wage(t)=(1-alpha)*prod_ag(t)*(km_ag(t)/labor_ag(t)/l_bar)^alpha;
      % wage
  %________________________________________________________________________
  %
  % Individual capital function, k'
  %________________________________________________________________________
   
  kprimet(:,1)=interpn(k,km,kprime(:,:,agshock(t),1),kvalues,km_ag(t)*ones(J,1),'linear'); 
      % interpolate the capital function k' (computed in "MAIN") of the 
      % unemployed agent in kvalues for the given agg. capital km_ag(t)
      
  kprimet(:,2)=interpn(k,km,kprime(:,:,agshock(t),2),kvalues,km_ag(t)*ones(J,1),'linear');
      % the same for the employed agent
      
   kprimet=kprimet.*(kprimet>=kvalues_min).*(kprimet<=kvalues_max)+kvalues_min*(kprimet<kvalues_min)+kvalues_max*(kprimet>kvalues_max);
      % restrict individual capital k' to be within [kvalues_min, kvalues_max]
      % range
   %_______________________________________________________________________
   %
   % Inverse of the individual capital function, k'(x) 
   %_______________________________________________________________________
   
   % To invert k'(x), we treat k' as an argument and x as a value of the 
   % function in the argument k'
   % Note that an inverse of k' is not well defined for those points of the 
   % grid for which the value of k' is the same (for example, for unemployed 
   % agents, we have k'=0 for several small grid values) 
   % Therefore, when inverting, we remove all but one grid points for which
   % the values of k' are repeated
   
   index_min=zeros(1, 2); % this variable will count how many times k'=kvalues_min 
      % in the employed and the unemployed states separately
      
   index_min=sum(kprimet==kvalues_min); % count how many times k'=kvalues_min
   
   first=index_min.*(index_min>0)+1*(index_min==0); % if index_min>0, consider  
      % k' starting from the (index_min)-th grid point; otherwise, consider  
      % k' starting from the first grid point 
      
   index_max=zeros(1, 2); % this variable will count how many times k'=kvalues_max 
      % in the employed and the unemployed states 
      
   index_max=sum(kprimet==kvalues_max); % count how many times k'=kvalues_max
   
   last=J-((index_max-1).*(index_max>0)+0*(index_max==0)); % if index_max>0, 
      %consider k' until the grid point (J-(index_max-1)); otherwise, 
      % consider k' until the last grid point, which is J 

   xt(:,1)=interp1(kprimet(first(1):last(1),1),kvalues(first(1):last(1),1), kvalues,'linear'); 
      % find x(k') in the unemployed state (state 1) by interpolation (see 
      % condition (10) in the paper)
      
   xt(:,2)=interp1(kprimet(first(2):last(2),2),kvalues(first(2):last(2),1), kvalues,'linear');
      % find x(k') in the employed state (state 2) by interpolation

   xt=xt.*(xt>=kvalues_min).*(xt<=kvalues_max)+kvalues_min*(xt<kvalues_min)+kvalues_max*(xt>kvalues_max);
      % restrict k to be in [kvalues_min, kvalues_max] 
      
   % Notice that some values of xt at the beginning and in the end of the 
   % grid will be NaN. This is because there are no values of kprimet 
   % (i.e., k') that correspond to some values of xt (i.e., x). 
   % For example, to have kprimet(xt)=0 for an employed agent xt must be 
   % negative, which is not allowed, so we get NaN. These NaN values of xt 
   % create a problem when computing the end-of-period capital distribution 
   % for terminal (but not for initial) grid-value points. To deal with this 
   % problem, we set xt in the end of the grid to kvalues_max whenever they 
   % are NaN. 

   % unemployed (consider xt(:,1))
   j=0; 
   while isnan(xt(J-j,1))==1; % consider xt=NaN from the end of the grid (j=0)
      xt(J-j,1)=kvalues_max;  % when xt=NaN, set xt=kvalues_max
      j=j+1;
   end

   % employed (consider xt(:,2))
   j=0; 
   while isnan(xt(J-j,2))==1;
      xt(J-j,2)=kvalues_max;
      j=j+1;
   end
%__________________________________________________________________________
%
% End-of-period cumulative capital distribution 
%__________________________________________________________________________

  Fu=zeros(J,1); % the end-of-period capital distribution for the unemployed; 
                 % note that we do not store the cumulative density over time   
  Fe=zeros(J,1); % the end-of-period capital distribution for the employed;

  for j=1:J; % we have J values of xt(j,1) and J values of xt(j,2) which are 
             % inverse of kprimet(kvalues) for the unemployed and employed
      
     % unemployed
              for i=1:J; % consider points on the grid (kvalues)
              if kvalues(i,1)<=xt(j,1); % if a grid point i considered is 
                  % <= xt(j,1)
                 Fu(j)=Fu(j)+kdistu(t,i); % then compute the cumulative 
                 % distribution Fu by adding the probability between the 
                 % points kvalues(i-1,1) and kvalues(i,1)                  
              end
              if kvalues(i,1)>xt(j,1); % if a grid point i considered is 
                  % >xt(j,1)
                 Fu(j)=Fu(j)+(xt(j,1)-kvalues(i-1,1))/(kvalues(i,1)-kvalues(i-1,1))*kdistu(t,i); break
                  % then compute the cumulative distribution Fe by adding 
                  % the probability to be the point between points kvalues(i-1,1) 
                  % and xt(j,1) and then break
              end     
       end
       
       % employed
              for i=1:J; 
              if kvalues(i,1)<=xt(j,2);
                  Fe(j)=Fe(j)+kdiste(t,i); 
               end
               if kvalues(i,1)>xt(j,2);
                 Fe(j)=Fe(j)+(xt(j,2)-kvalues(i-1,1))/(kvalues(i,1)-kvalues(i-1,1))*kdiste(t,i); break  
               end
         end
  end
%__________________________________________________________________________
%
% Next period's beginning-of-period distribution
%__________________________________________________________________________
 
if t < T % we do not compute next period's beginning-of-period distributions 
         % for t=T (i.e., kdistu(T+1,:) and kdiste(T+1,:)) as we do not have 
         % agshock(T+1) 
     
%  Mass of agents in different idiosyncratic states conditional on agg.
%  states

   if (agshock(t)==1)&(agshock(t+1)==1);g=p_bb; end % g is a 2*2 matrix; 
   % for example, g(1,2) is the mass of agents who were unemployed at t and 
   % employed at t+1, conditional on being in a bad agg. state at both t 
   % and t+1
   if (agshock(t)==1)&(agshock(t+1)==2);g=p_bg; end
   if (agshock(t)==2)&(agshock(t+1)==1);g=p_gb; end
   if (agshock(t)==2)&(agshock(t+1)==2);g=p_gg; end

% Next period's beginning-of-period distribution (see formulas in Den Haan, 
% Judd and Juillard, 2008)    
   
   Pu=(g(1,1)*Fu+g(2,1)*Fe)/(g(1,1)+g(2,1)); % for the unemployed
   Pe=(g(1,2)*Fu+g(2,2)*Fe)/(g(1,2)+g(2,2)); % for the employed
   
  % unemployed 
   kdistu(t+1,1)=Pu(1,1); % probability of having k=kvalues_min at t+1
   kdistu(t+1,2:J-1)=Pu(2:J-1,1)'-Pu(1:J-2,1)'; % probabilities of different 
                                                % grid points kvalues
   kdistu(t+1,J)=1-sum(kdistu(t+1,1:J-1));      % probability of k=kvalues_max 
                                                % is set so that "kdistu" is
                                                % normalized to one
     
  % employed 
   kdiste(t+1,1)=Pe(1,1); 
   kdiste(t+1,2:J-1)=Pe(2:J-1,1)'-Pe(1:J-2,1)';
   kdiste(t+1,J)=1-sum(kdiste(t+1,1:J-1));
 end
end; % end of the NON-STOCHASTIC SIMULATION


case 1
%__________________________________________________________________________
%
% 4. STOCHASTIC SIMULATION
%__________________________________________________________________________

% Time series of aggregate variables

km_ag=zeros(T,1);       % aggregate capital (mean of capital distribution)
kcross=zeros(1,N)+ k_ind(1,1);   % initial capital of all agents is equal to 43

for t=1:T
    km_ag(t)=mean(kcross); % find the t-th observation of kmts by computing 
                         % the mean of the t-th period cross-sectional 
                         % distribution of capital
   km_ag(t)=km_ag(t)*(km_ag(t)>=km_min)*(km_ag(t)<=km_max)+km_min*(km_ag(t)<km_min)+km_max*(km_ag(t)>km_max); % restrict kmts to be within [km_min, km_max]
   
   % To find kmts(t+1), we should compute a new cross-sectional distribution 
   % at t+1. For this purpose, we first find kprime by interpolation for 
   % realized kmts(t) and agshock(t) (kprimet below) and then use it to 
   % compute new kcross by interpolation (kcrossn below) given the previous 
   % kcross and the realized idshock(t)
   
   kprimet4=interpn(k,km,a2,epsilon2,kprime,k, km_ag(t),agshock(t),epsilon2,'spline');
      % a four-dimensional capital function at time t is obtained by fixing
      % known kmts(t) and agshock (t)
      
   kprimet=squeeze(kprimet4); % the size of kprimet4 is ngridk*1*1*nstates_id; 
                              % in kprimet, all singleton dimensions (i.e.,
                              % those with only one column per page) are removed
                              
   kcrossn=interpn(k,epsilon2,kprimet,kcross,idshock(t,:),'spline'); 
                              % given kcross and idiosyncratic shocks we
                              % compute kcrossn
                              
   kcrossn=kcrossn.*(kcrossn>=k_min).*(kcrossn<=k_max)+k_min*(kcrossn<k_min)+k_max*(kcrossn>k_max); % restrict kcross to be within [k_min, k_max]
                              
   kcross=kcrossn;
end

end% end switch-case simulation_type


%__________________________________________________________________________
%
% 5. QUANTITIES
%__________________________________________________________________________

for t=1:T-1; 

 irate(t)=alpha*prod_ag(t)*(km_ag(t)/labor_ag(t)/l_bar)^(alpha-1);
      % interest rate 
 wage(t)=(1-alpha)*prod_ag(t)*(km_ag(t)/labor_ag(t)/l_bar)^alpha;
      
% Individual capital

k_ind(t+1,1)=interpn(k,km,kprime(:,:,agshock(t),idshock(t,1)),k_ind(t,1),km_ag(t),'linear');

k_ind(t+1,1)=k_ind(t+1,1).*(k_ind(t+1,1)>=k_min).*(k_ind(t+1,1)<=k_max)+k_min*(k_ind(t+1,1)<k_min)+k_max*(k_ind(t+1,1)>k_max);
           % restrict k_ind to be in [k_min,k_max] range

% Individual income

income_ind(t,1)=k_ind(t,1)*irate(t)+(idshock(t,1)-1).*l_bar*wage(t)+mu*(2-idshock(t,1)).*wage(t)-...
ur(agshock(t))/(1-ur(agshock(t)))*mu*(idshock(t,1)-1).*wage(t);
 
% Individual consumption

c_ind(t,1)=k_ind(t,1)*(1-delta+irate(t))+(idshock(t,1)-1).*l_bar*wage(t)+mu*(2-idshock(t,1)).*wage(t)-...
ur(agshock(t))/(1-ur(agshock(t)))*mu*(idshock(t,1)-1).*wage(t)-k_ind(t+1,1);

end 

% Output
output=prod_ag(1:T-1,1).*km_ag(1:T-1,1).^alpha.*(labor_ag(1:T-1,1)*l_bar).^(1-alpha);

% Aggregate consumption
c_ag=km_ag(1:T-1,1)*(1-delta)+output(1:T-1,1)-km_ag(2:T,1);

% Aggregate income
income_ag=output(1:T-1,1);

% Aggregate investment
i_ag=km_ag(2:T)-(1-delta)*km_ag(1:T-1);

%__________________________________________________________________________
%
% 9. EULER EQUATION ERRORS ON SIMULATED TIME PATH
%__________________________________________________________________________

c_eeq=zeros(T-1,1); % individual consumption computed from the Euler 
                    % equation using true individual and aggregate capital 

   for t=1:T-1
       
      % Future (at t+1) interest rate 
      
      irate_b=alpha*a(1)*(km_ag(t+1)/er_b/l_bar)^(alpha-1);
      irate_g=alpha*a(2)*(km_ag(t+1)/er_g/l_bar)^(alpha-1);
      
      % Future (at t+1) wage
      
      wage_b=(1-alpha)*a(1)*(km_ag(t+1)/er_b/l_bar)^(alpha);
      wage_g=(1-alpha)*a(2)*(km_ag(t+1)/er_g/l_bar)^(alpha);
      
      % Future (at t+2) value of capital (i.e., k'') in four possible
      % states (temporary variable)
      
      k_hat1_bu=interpn(k,km,a2,epsilon2,kprime,k_ind(t+1,1),km_ag(t+1),1,1,'linear');
      k_hat1_be=interpn(k,km,a2,epsilon2,kprime,k_ind(t+1,1),km_ag(t+1),1,2,'linear');
      k_hat1_gu=interpn(k,km,a2,epsilon2,kprime,k_ind(t+1,1),km_ag(t+1),2,1,'linear');      
      k_hat1_ge=interpn(k,km,a2,epsilon2,kprime,k_ind(t+1,1),km_ag(t+1),2,2,'linear');      
     
      % Current consumption computed from the Euler equation
      
      curr_state=(agshock(t)-1)*2+idshock(t,1); % supplementary variable for 
                                                % computing the transition 
                                                % probabilities from "prob" 
      
      c_eeq(t,1)=(beta*(((1-delta+irate_b)*k_ind(t+1,1)+wage_b*l_bar*epsilon_u+mu*wage_b*(1-epsilon_u)-mu*wage_b*ur_b/(1-ur_b)*epsilon_u-k_hat1_bu)^(-gamma)*(1-delta+irate_b)*prob(curr_state,1)...
         +((1-delta+irate_b)*k_ind(t+1,1)+wage_b*l_bar*epsilon_e+mu*wage_b*(1-epsilon_e)-mu*wage_b*ur(1)/(1-ur(1))*epsilon_e-k_hat1_be)^(-gamma)*(1-delta+irate_b)*prob(curr_state,2)...
         +((1-delta+irate_g)*k_ind(t+1,1)+wage_g*l_bar*epsilon_u+mu*wage_g*(1-epsilon_u)-mu*wage_g*ur(2)/(1-ur(2))*epsilon_u-k_hat1_gu)^(-gamma)*(1-delta+irate_g)*prob(curr_state,3)...
         +((1-delta+irate_g)*k_ind(t+1,1)+wage_g*l_bar*epsilon_e+mu*wage_g*(1-epsilon_e)-mu*wage_g*ur(2)/(1-ur(2))*epsilon_e-k_hat1_ge)^(-gamma)*(1-delta+irate_g)*prob(curr_state,4)))^(-1/gamma);
          
      % Future (at j+1) capital (i.e., k') restored from the budget constraint 
      
      k_eeq=k_ind(t,1)*(1-delta+irate(t))+(idshock(t,1)-1).*l_bar*wage(t)+mu*(2-idshock(t,1)).*wage(t)-...
      ur(agshock(t))/(1-ur(agshock(t)))*mu*(idshock(t,1)-1).*wage(t)-c_eeq(t,1);
  
      % Check that k_eeq is not negative 
      
      k_eeq=k_eeq*(k_eeq>0); % set k_eeq=0 if it is negative
  
      % Re-compute c_eeq whenever k_eeq was initially negative 
      
      c_eeq(t,1)=k_ind(t,1)*(1-delta+irate(t))+(idshock(t,1)-1).*l_bar*wage(t)+mu*(2-idshock(t,1)).*wage(t)-...
      ur(agshock(t))/(1-ur(agshock(t)))*mu*(idshock(t,1)-1).*wage(t)-k_eeq;

   end
   
% Table 1 in Maliar, Maliar and Valli's (2008)

c_error=abs((c_ind(1:T-1,1)-c_eeq)./c_eeq)*100; % percentage difference 
                                                % between c_eeq and c_ind
Table1(1,1)= {'average'};Table1(1,2)={mean(c_error')};
Table1(2,1)= {'maximal'};Table1(2,2)={max(c_error')};
%__________________________________________________________________________
%
% 10. RESULTS
%__________________________________________________________________________

% Results corresponding to Tables 9,11,12,14,15,16 in Den Haan (2008) and
% Table 1 in Maliar, Maliar and Valli (2008)
%{
disp('Table 9. Means and standard deviations of cross-sectional moments'); Table9
disp('Table 11. Means of the 5th and 10th percentile'); Table11
disp('Table 12. Properties of individual policy rules'); Table12
disp('Table 14. Percentage errors from dynamic Euler accuracy test'); Table14
disp('Table 15. Moments of Kt in panel and according to aggregate law of motion'); Table15
disp('Table 16. Accuracy aggregate policy rule'); Table16
%}
%disp('Table 1.  Euler equation errors'); Table1

end