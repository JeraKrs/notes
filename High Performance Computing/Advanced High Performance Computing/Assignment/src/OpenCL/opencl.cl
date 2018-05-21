#pragma OPENCL EXTENSION cl_khr_fp64 : enable

typedef struct
{
    int    nx;
    int    ny;
    int    maxIters;
    int    reynolds_dim;
    float density;
    float accel;
    float omega;
	int tt;
	int groupSize;
} t_param;

#define NSPEEDS         9
typedef float t_speed;

__kernel void flow(__global			t_speed		*cells,
				   __global const	int			*obstacles,
							const	t_param		params)
{
    float w1 = params.density * params.accel / 9.f;
    float w2 = params.density * params.accel / 36.f;

	int ii = get_global_id(0);
	int col = (params.ny - 2) * params.nx + ii; 
	int bsize = params.nx * params.ny;

	if (!obstacles[col]
			&& (cells[col+3*bsize] - w1) > 0.f 
			&& (cells[col+6*bsize] - w2) > 0.f 
			&& (cells[col+7*bsize] - w2) > 0.f)
	{   
		cells[col+1*bsize] += w1; 
		cells[col+5*bsize] += w2; 
		cells[col+8*bsize] += w2; 
		cells[col+3*bsize] -= w1; 
		cells[col+6*bsize] -= w2; 
		cells[col+7*bsize] -= w2; 
	} 
}

__kernel void move(__global	const	t_speed *cells,
				   __global			t_speed *tmp_cells,
				   __global	const	int		* obstacles,
							const	t_param params,
				   __local			float	* sums,
				   __global			float	* ret)
{
    float w0 = 4.f / 9.f;
    float w1 = 1.f / 9.f;
    float w2 = 1.f / 36.f;

	int jj = get_global_id(1);
	int ii = get_global_id(0);
	int kk;
	int col = jj*params.nx + ii;
	int bsize = params.nx * params.ny;

	float tot_u = 0.f;
	__private float tmp[NSPEEDS];
	int y_n = (jj + 1) % params.ny;
	int x_e = (ii + 1) % params.nx;
	int y_s = (jj == 0) ? (jj + params.ny - 1) : (jj - 1);
	int x_w = (ii == 0) ? (ii + params.nx - 1) : (ii - 1);

	tmp[0] = cells[ii + jj*params.nx+0*bsize];
	tmp[1] = cells[x_w + jj*params.nx+1*bsize];
	tmp[2] = cells[ii + y_s*params.nx+2*bsize];
	tmp[3] = cells[x_e + jj*params.nx+3*bsize];
	tmp[4] = cells[ii + y_n*params.nx+4*bsize];
	tmp[5] = cells[x_w + y_s*params.nx+5*bsize];
	tmp[6] = cells[x_e + y_s*params.nx+6*bsize];
	tmp[7] = cells[x_e + y_n*params.nx+7*bsize];
	tmp[8] = cells[x_w + y_n*params.nx+8*bsize];

	if (obstacles[col])
	{
		tmp_cells[col+1*bsize] = tmp[3];
		tmp_cells[col+2*bsize] = tmp[4];
		tmp_cells[col+3*bsize] = tmp[1];
		tmp_cells[col+4*bsize] = tmp[2];
		tmp_cells[col+5*bsize] = tmp[7];
		tmp_cells[col+6*bsize] = tmp[8];
		tmp_cells[col+7*bsize] = tmp[5];
		tmp_cells[col+8*bsize] = tmp[6];
	}
	else
	{
		float local_density = 0.f;

		for (kk = 0; kk < NSPEEDS; kk++)
		{
			local_density += tmp[kk];
		}

		float u_x = (tmp[1] + tmp[5] + tmp[8]
				- (tmp[3] + tmp[6] + tmp[7])) / local_density;
		float u_y = (tmp[2] + tmp[5] + tmp[6]
				- (tmp[4] + tmp[7] + tmp[8])) / local_density;

		float u_sq = u_x * u_x + u_y * u_y;
		float u[NSPEEDS];
		u[1] =   u_x;
		u[2] =         u_y;
		u[3] = - u_x;
		u[4] =       - u_y;
		u[5] =   u_x + u_y;
		u[6] = - u_x + u_y;
		u[7] = - u_x - u_y;
		u[8] =   u_x - u_y;

		float d_equ[NSPEEDS];
		d_equ[0] = w0 * local_density * (1.f - u_sq * 1.5f);
		d_equ[1] = w1 * local_density * (1.f + u[1] * 3.f
						+ (u[1] * u[1]) * 4.5f - u_sq * 1.5f);
		d_equ[2] = w1 * local_density * (1.f + u[2] * 3.f
						+ (u[2] * u[2]) * 4.5f - u_sq * 1.5f);
		d_equ[3] = w1 * local_density * (1.f + u[3] * 3.f
						+ (u[3] * u[3]) * 4.5f - u_sq * 1.5f);
		d_equ[4] = w1 * local_density * (1.f + u[4] * 3.f
						+ (u[4] * u[4]) * 4.5f - u_sq * 1.5f);
		d_equ[5] = w2 * local_density * (1.f + u[5] * 3.f
						+ (u[5] * u[5]) * 4.5f - u_sq * 1.5f);
		d_equ[6] = w2 * local_density * (1.f + u[6] * 3.f
						+ (u[6] * u[6]) * 4.5f - u_sq * 1.5f);
		d_equ[7] = w2 * local_density * (1.f + u[7] * 3.f
						+ (u[7] * u[7]) * 4.5f - u_sq * 1.5f);
		d_equ[8] = w2 * local_density * (1.f + u[8] * 3.f
						+ (u[8] * u[8]) * 4.5f - u_sq * 1.5f);

		local_density = 0.f;
		for (kk = 0; kk < NSPEEDS; kk++)
		{
			tmp[kk] = tmp[kk] + params.omega * (d_equ[kk] - tmp[kk]);
			local_density += tmp[kk];
			tmp_cells[col+kk*bsize] = tmp[kk];
		}

		u_x = (tmp[1] + tmp[5] + tmp[8]
			- (tmp[3] + tmp[6] + tmp[7])) / local_density;
		u_y = (tmp[2] + tmp[5] + tmp[6]
			- (tmp[4] + tmp[7] + tmp[8])) / local_density;
		tot_u += sqrt((u_x * u_x) + (u_y * u_y));
	}

	int ySize = get_local_size(0);
	int xSize = get_local_size(1);
	int yNum = get_local_id(0);
	int xNum = get_local_id(1);

	int xgSize = get_num_groups(1);
	int ygNum = get_group_id(0);
	int xgNum = get_group_id(1);

	int tNum = yNum*xSize + xNum;
	int wgNum = ygNum*xgSize + xgNum + params.tt * params.groupSize;

	int numItems = xSize * ySize;
	sums[tNum] = tot_u;
	for(kk = numItems/2; kk>0; kk >>= 1) {
		if(tNum < kk) {
			barrier(CLK_LOCAL_MEM_FENCE);
			sums[tNum] += sums[tNum+kk];
		}
	}
	if(tNum == 0)
		ret[wgNum] = sums[tNum];
}
