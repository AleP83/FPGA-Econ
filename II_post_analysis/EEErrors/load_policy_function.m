function kprime= load_policy_function(file_path,ngridk,ngridkm,nA,nepsilon)
%LOAD POLICY FUNCTION Load policy function estimated on different ASICS
kprime_vec_C = load(file_path);
% Format from C to Matlab: IXV(a,km,id,k) (NKM_GRID*NSTATES_ID*NKGRID*a+NSTATES_ID*NKGRID*km+NKGRID*id+k)
for ia = 1:nA        
    for ikm = 1:ngridkm
        for ie = 1:nepsilon
            for ik=1:ngridk
                % Position in array C
                is = ngridkm*nepsilon*ngridk*(ia-1)+nepsilon*ngridk*(ikm-1)+ngridk*(ie-1)+(ik-1);
                is_M = is+1;
                % interpn(k,km,a2,epsilon2,kprime,k_hat(t+1,1),kmalm_hat(t+1),1,1,'linear')
                kprime(ik,ikm,ia,ie) = kprime_vec_C(is_M);
            end 
        end 
    end
end 

end