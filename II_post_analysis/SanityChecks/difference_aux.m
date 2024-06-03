function R_ = difference_aux(A,B)
abs_diff = abs(A- B);
R_.max_value= max(abs_diff,[],'all');
R_.mean_value = mean(abs_diff,'all');

%% Python equal = np.allclose(A, B, rtol=1e-05, atol=1e-09, equal_nan=False)
%  This function evaluates whether the elements of A and B are element-wise close to each other within specified tolerance levels. 
% The parameters rtol and atol define relative and absolute tolerances, respectively. 
% If the absolute difference between each element of A and B is less than or equal to atol + rtol * abs(B), the arrays are considered close, and equal is set to True. 
% Otherwise, equal is set to False. The argument equal_nan=False specifies that NaNs (Not a Number) in the same position in both arrays do not count as equal.
atol = 1e-09;
rtol = 1e-05;
equal = (sum(abs_diff<=(atol+rtol*abs(B)),'all')==numel(abs_diff));
assert(equal == 1)
R_.equal = equal;
end