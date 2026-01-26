//////////////////////////////////////////////////////////////////////
//
//	NxM Matrix header
//	
//	File: matrixnm.h
//	Description : matrixnm template class declaration and inlined implementation
//
//	History:
//	-:Created by Anton Knyazev
//
//////////////////////////////////////////////////////////////////////

#ifndef matrixnm_h
#define matrixnm_h
#pragma once

enum 	mtxflags { 
	mtx_invalid=1, mtx_normal=2, mtx_orthogonal=4, mtx_PSD=8, mtx_PD_flag=16, mtx_PD=mtx_PSD|mtx_PD_flag, mtx_symmetric=32, 
	mtx_diagonal_flag=64, mtx_diagonal=mtx_symmetric|mtx_normal|mtx_diagonal_flag, mtx_identity_flag=128, 
	mtx_identity=mtx_PD|mtx_diagonal|mtx_orthogonal|mtx_normal|mtx_symmetric|mtx_identity_flag, mtx_singular=256, 
	mtx_foreign_data=1024, // this flag means that data was not allocated by matrix
	mtx_allocate=32768 // prohibts using matrix pool for data
};

#include "vectorn.h"

template<class ftype> class matrix_product_tpl {
public:
	matrix_product_tpl(int nrows1,int ncols1,ftype *pdata1,int flags1, int ncols2,ftype *pdata2,int flags2) {
		data1=pdata1; data2=pdata2; nRows1=nrows1;nCols1=ncols1; nCols2=ncols2;
		flags = flags1 & flags2 & (mtx_orthogonal | mtx_PD) & ~mtx_foreign_data;
	}
	void assign_to(ftype *pdst) const {
		int i,j,k; ftype sum;
		for(i=0;i<nRows1;i++) for(j=0;j<nCols2;j++) {
			for(sum=0,k=0; k<nCols1; k++) 
				sum += data1[i*nCols1+k]*data2[k*nCols2+j];
			pdst[i*nCols2+j] = sum;
		}
	}
	void add_assign_to(ftype *pdst) const {
		for(int i=0;i<nRows1;i++) for(int j=0;j<nCols2;j++)
			for(int k=0; k<nCols1; k++) 
				pdst[i*nCols2+j] += data1[i*nCols1+k]*data2[k*nCols2+j];
	}
	void sub_assign_to(ftype *pdst) const {
		for(int i=0;i<nRows1;i++) for(int j=0;j<nCols2;j++)
			for(int k=0; k<nCols1; k++) 
				pdst[i*nCols2+j] -= data1[i*nCols1+k]*data2[k*nCols2+j];
	}

	int nRows1,nCols1,nCols2;
	ftype *data1,*data2;
	int flags;
};

inline float getlothresh(float) { return 1E-10f; }
inline float gethithresh(float) { return 1E10f; }
inline double getlothresh(double) { return 1E-20; }
inline double gethithresh(double) { return 1E20; }

template <class ftype> class matrix_tpl { 
public:
	matrix_tpl() {
		nRows=nCols=3; flags=mtx_foreign_data; data=0;
	}
	matrix_tpl( int nrows, int ncols, int _flags=0, ftype *pdata=(ftype*)-1 ) {
		nRows = nrows; nCols = ncols;
		flags = _flags & ~mtx_allocate;
		int sz = nRows*nCols;
		if (pdata!=(ftype*)-1) {
			data = pdata; flags |= mtx_foreign_data;
#ifdef USE_MATRIX_POOLS
		} else if (sz<=36 && !(_flags & mtx_allocate)) {
			if (mtx_pool_pos+sz > mtx_pool_size)
				mtx_pool_pos = 0;
			data = mtx_pool+mtx_pool_pos;
			mtx_pool_pos += sz;
			flags |= mtx_foreign_data;
#endif
		} else
			data = new ftype[sz];
	}
	matrix_tpl(const matrix_tpl& src) {
		if (src.flags & mtx_foreign_data) {
			nRows=src.nRows; nCols=src.nCols;
			flags=src.flags; data=src.data;
		} else {
			matrix_tpl(src.nRows,src.nCols,src.flags,0);
			for(int i=nRows*nCols-1;i>=0;i--) data[i]=src.data[i];
		}
	}
	~matrix_tpl() {
		if (data && !(flags & mtx_foreign_data)) delete [] data;
	}

	matrix_tpl& operator=(const matrix_tpl<ftype> &src) {
		if (!data || !(flags & mtx_foreign_data) && nRows*nCols<src.nRows*src.nCols) {
			delete[] data; data = new ftype[src.nRows*src.nCols];
		}
		nRows=src.nRows; nCols=src.nCols; 
		flags = flags&mtx_foreign_data | src.flags&~mtx_foreign_data;
		for(int i=nRows*nCols-1;i>=0;i--) data[i] = src.data[i];
		return *this;
	}
	template<class ftype1> matrix_tpl& operator=(const matrix_tpl<ftype1> &src) {
		if (!data || !(flags & mtx_foreign_data) && nRows*nCols<src.nRows*src.nCols) {
			delete[] data; data = new ftype[src.nRows*src.nCols];
		}
		nRows=src.nRows; nCols=src.nCols; 
		flags = flags&mtx_foreign_data | src.flags&~mtx_foreign_data;
		for(int i=nRows*nCols-1;i>=0;i--) data[i] = src.data[i];
		return *this;
	}

	matrix_tpl& operator=(const matrix_product_tpl<ftype> &src) {
		nRows=src.nRows1; nCols=src.nCols2; 
		flags = flags&mtx_foreign_data | src.flags;
		src.assign_to(data);
		return *this;
	}
	matrix_tpl& operator+=(const matrix_product_tpl<ftype> &src) {
		src.add_assign_to(data); return *this;
	}
	matrix_tpl& operator-=(const matrix_product_tpl<ftype> &src) {
		src.sub_assign_to(data); return *this;
	}

	matrix_tpl& allocate() {
		int i,sz=nRows*nCols; ftype *prevdata = data;
		if (!data) data = new ftype[sz];
		if (flags & mtx_foreign_data) for(i=0;i<sz;i++) data[i] = prevdata[i];
		return *this;
	}

	matrix_tpl& zero() { for(int i=nRows*nCols-1;i>=0;i--) data[i]=0; return *this; }
	matrix_tpl& identity() {
		zero(); for(int i=min(nRows,nCols)-1;i>=0;i--) data[i*(nCols+1)] = 1;
		return *this;
	}
	
	matrix_tpl& invert() // in-place inversion
	{
		if (flags & mtx_orthogonal)
			return transpose();
		if (nRows!=nCols)
			return *this;
		
		int i,j; ftype det=0;
		if (nRows==1)
			data[0]=(ftype)1.0/data[0];
		else if (nRows==2) {
			det = data[0]*data[3]-data[1]*data[2];
			if (det==0) return *this;	det = (ftype)1.0/det;
			ftype oldata[4]; for(i=0;i<4;i++) oldata[i]=data[i];
			data[0]=oldata[3]*det; data[1]=-oldata[1]*det;
			data[2]=-oldata[2]*det; data[3]=oldata[0]*det;
		} else if (nRows==3) {
			for(i=0;i<3;i++) det+=data[i]*data[inc_mod3[i]+3]*data[dec_mod3[i]+6];
			for(i=0;i<3;i++) det-=data[dec_mod3[i]]*data[inc_mod3[i]+3]*data[i+6];
			if (det==0) return *this;	det = (ftype)1.0/det;
			ftype oldata[9]; for(i=0;i<9;i++) oldata[i]=data[i];
			for(i=0;i<3;i++) for(j=0;j<3;j++)
				data[i+j*3] = (oldata[dec_mod3[i]*3+dec_mod3[j]]*oldata[inc_mod3[i]*3+inc_mod3[j]]-
											oldata[dec_mod3[i]*3+inc_mod3[j]]*oldata[inc_mod3[i]*3+dec_mod3[j]])*det;
		} else {
			ftype *LUdata=0,*colmark; int *LUidx=0,LUidx_buf[32], alloc=0;
			if (nRows*nRows*2<mtx_pool_size) {
				if (mtx_pool_pos+nRows*nRows > mtx_pool_size)
					mtx_pool_pos = 0;
				LUdata = mtx_pool+mtx_pool_pos; mtx_pool_pos += nRows*nRows;
			} else alloc = 1;
			if (nRows<=sizeof(LUidx_buf)/sizeof(LUidx_buf[0])) LUidx = LUidx_buf;
			else alloc |= 2;
			if (!LUdecomposition(LUdata,LUidx)) return *this;

			if (nRows*2<mtx_pool_size) {
				if (mtx_pool_pos+nRows > mtx_pool_size)
					mtx_pool_pos=0;
				colmark=mtx_pool+mtx_pool_pos; mtx_pool_pos+=nRows;
			} else {
				colmark = new ftype[nRows]; alloc |= 4;
			}

			for(i=0;i<nRows;i++) colmark[i]=0;
			for(i=0;i<nRows;i++) {
				colmark[i]=1;	solveAx_b(data+i*nRows,colmark, LUdata,LUidx); colmark[i]=0;
			}
			transpose();

			if (alloc & 1) delete[] LUdata;
			if (alloc & 2) delete[] LUidx;
			if (alloc & 4) delete[] colmark;
		}
		flags = flags & (mtx_normal | mtx_orthogonal | mtx_symmetric | mtx_PD | mtx_PSD | mtx_diagonal | mtx_identity | mtx_foreign_data);
		return *this;
	}
	matrix_tpl operator!() const { // returns inverted matrix
		if (flags & mtx_orthogonal)
			return T();
		matrix_tpl<ftype> res = *this;
		res.invert();
		return res;
	}

	matrix_tpl& transpose() { // in-place transposition
		if (nRows==nCols) {
			if (flags & mtx_symmetric) return *this;
			int i,j; ftype t; for(i=0;i<nRows;i++) for(j=0;j<i;j++) {
				t=(*this)[i][j]; (*this)[i][j]=(*this)[j][i]; (*this)[j][i]=t; 
			}
		} else 
			*this = T();
		return *this;
	}
	matrix_tpl T() const { // returns transposed matrix
		if (flags & mtx_symmetric) 
			return matrix_tpl<ftype>(*this);
		int i,j; matrix_tpl<ftype> res(nCols,nRows, flags & ~mtx_foreign_data);
		for(i=0;i<nRows;i++) for(j=0;j<nCols;j++) res[j][i] = (*this)[i][j];
		return res;
	}

	int LUdecomposition(ftype *&LUdata,int *&LUidx) const;
	int solveAx_b(ftype *x,ftype *b, ftype *LUdata=0,int *LUidx=0) const // finds x that satisfies Ax=b
	{
	int LUidx_buf[16],alloc=0;
	if (!LUdata) {
		if (nRows*nRows*2<mtx_pool_size) {
			if (mtx_pool_pos+nRows*nRows > mtx_pool_size)
				mtx_pool_pos = 0;
			LUdata = mtx_pool+mtx_pool_pos; mtx_pool_pos += nRows*nRows;
		}
		if (nRows<=sizeof(LUidx_buf)/sizeof(LUidx_buf[0])) LUidx = LUidx_buf;
		alloc = LUdata==0 | (LUidx==0)<<1;
		if (!LUdecomposition(LUdata,LUidx)) return 0;
	}

	int i,j; ftype xi;
	matrix_tpl<ftype> LU(nRows,nRows,0,LUdata);
	for(i=0;i<nRows;i++) x[i]=b[i];
	for(i=0;i<nRows;i++) {
		xi=x[i]; x[i]=x[LUidx[i]]; x[LUidx[i]]=xi;
		for(j=0;j<i;j++) x[i]-=LU[i][j]*x[j];
	}
	for(i=nRows-1;i>=0;i--) {
		for(j=i+1;j<nRows;j++) x[i]-=LU[i][j]*x[j];
		x[i] /= LU[i][i];
	}

	if (alloc & 1) delete[] LUdata;
	if (alloc & 2) delete[] LUidx;
	return 1;
}
	ftype determinant(ftype *LUdata=0,int *LUidx=0) const;

	int jacobi_transformation(matrix_tpl &evec, ftype* eval, ftype prec=0) const
	{
		if (!(flags & mtx_symmetric) || nCols!=nRows) return 0;

	matrix_tpl a(*this);
	int n = nRows, p,q,r,iter,pmax,qmax,sz=nRows*nCols;
	ftype theta,t,s,c,apr,aqr,arp,arq,thresh=prec,amax;
	evec.identity();
	evec.flags = (evec.flags & mtx_foreign_data) | mtx_orthogonal | mtx_normal;
	
	for(iter=0; iter<nRows*nCols*10; iter++) {
		for(p=0,amax=thresh,pmax=-1;p<n-1;p++) for(q=p+1;q<n;q++)	if (sqr(a[p][q])>amax) 
		{	amax=sqr(a[p][q]); pmax=p; qmax=q; }
		if (pmax==-1) 
			goto exitjacobi;
		p=pmax; q=qmax;
		theta = (ftype)0.5*(a[q][q]-a[p][p])/a[p][q];
		if (fabs_tpl(theta)<gethithresh(theta)) {
			t = sqrt_tpl(theta*theta+1);
			if (theta>0) t = -theta-t;
			else t -= theta;
			c = 1/sqrt_tpl(1+t*t); s = t*c;
			for(r=0;r<n;r++) { arp=a[r][p];arq=a[r][q]; a[r][p]=c*arp-s*arq; a[r][q]=c*arq+s*arp; }
			for(r=0;r<n;r++) { apr=a[p][r];aqr=a[q][r]; a[p][r]=c*apr-s*aqr; a[q][r]=c*aqr+s*apr; }
			for(r=0;r<n;r++) { apr=evec[p][r];aqr=evec[q][r]; evec[p][r]=c*apr-s*aqr; evec[q][r]=c*aqr+s*apr; }
		}
		a[p][q] = 0;
		if (iter==sz+1) thresh += getlothresh(thresh);
	}
	iter=0;
	exitjacobi:
	for(p=0;p<n*n;p++) {
		t = fabs_tpl(evec.data[p]);
		if (t<(ftype)1E-6) evec.data[p]=0;
		else if (fabs_tpl(t-1)<getlothresh(t)) 
			evec.data[p]=sgnnz(evec.data[p]);
	}
	for(p=0;p<n;p++) eval[p] = a[p][p];
	return iter; // not converged during iterations limit
	}
	int conjugate_gradient(ftype *startx,ftype *rightside, ftype minlen=0,ftype minel=0) const
	{
		ftype a,b,r2,r2new,denom,maxel;
		int i,iter=nRows*3;
		minlen*=minlen; 

		ftype buf[24],*pbuf = nRows>8 ? new ftype[nRows*3]:buf;
		vectorn_tpl<ftype> x(nRows,startx),rh(nRows,rightside),r(nRows,pbuf),p(nRows,pbuf+nRows),Ap(nRows,pbuf+nRows*2);
		(r=rh)-=*this*x; p=r;
		r2=r.len2();

		do {
			Ap = *this*p; denom = p*Ap;
			if (sqr(denom)<1E-30) break;
			a = r2/denom;	
	#ifdef __GNUC__
			vectorn_tpl<ftype> ApXa(nRows, Ap * a);
			r -= ApXa;
	#else
			r -= Ap*a;
	#endif
			r2new=r.len2();
			if (r2new>r2*500)
				break;
	#ifdef __GNUC__
			vectorn_tpl<ftype> pXa(nRows, p * a);
			x += pXa;
	#else
			x += p*a;
	#endif
			b = r2new/r2; r2=r2new;
			(p*=b)+=r;
			for(i=0,maxel=0;i<nRows;i++) maxel = max(maxel,fabs_tpl(r[i]));
		} while (--iter && (r2new>minlen || maxel>minel));

		if (pbuf!=buf) delete pbuf;
		return nRows-iter;
	}
	int biconjugate_gradient(ftype *startx,ftype *rightside, ftype minlen=0,ftype minel=0) const;
	int minimum_residual(ftype *startx,ftype *rightside, ftype minlen=0,ftype minel=0) const;
	int LPsimplex(int m1,int m2, ftype &objfunout,ftype *xout=0, int nvars=-1, ftype e=-1) const;

	ftype *operator[](int iRow) const { return data + iRow*nCols; }

	int nRows,nCols;
	int flags;
	ftype *data;

#ifdef USE_MATRIX_POOLS
	static ftype mtx_pool[];
	static int mtx_pool_pos;
	static int mtx_pool_size;
#endif
};

/*template<class ftype1,class ftype2>
matrix_tpl<ftype1> operator*(const matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	matrix_tpl<ftype1> res(op1.nRows, op2.nCols);
	res.flags = res.flags & mtx_foreign_data | op1.flags & op2.flags & (mtx_orthogonal | mtx_PD) & ~mtx_foreign_data;
	int i,j,k; ftype1 sum;
	for(i=0;i<op1.nRows;i++) for(j=0;j<op2.nCols;j++) {
		for(sum=0,k=0; k<op1.nCols; k++) sum += op1[i][k]*op2[k][j];
		res[i][j] = sum;
	}
	return res;
}*/

template<class ftype>
matrix_product_tpl<ftype> operator*(const matrix_tpl<ftype> &op1, const matrix_tpl<ftype> &op2) {
	return matrix_product_tpl<ftype>(op1.nRows,op1.nCols,op1.data,op1.flags, op2.nCols,op2.data,op2.flags);
}

/*template<class ftype1,class ftype2>
matrix_tpl<ftype1>& operator*=(matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	return op1 = (op1 * op2);
}*/

template<class ftype>
matrix_tpl<ftype>& operator*=(matrix_tpl<ftype> &op1,ftype op2) {
	for(int i=op1.nRows*op1.nCols-1; i>=0; i--) op1.data[i]*=op2;
	op1.flags &= ~(mtx_identity_flag | mtx_PD);
	return op1;
}

template<class ftype1,class ftype2>
matrix_tpl<ftype1>& operator+=(matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	for(int i=op1.nRows*op1.nCols-1; i>=0; i--) op1.data[i] += op2.data[i];
	op1.flags = op1.flags & mtx_foreign_data | op1.flags & op2.flags & (mtx_symmetric | mtx_PD);
	return op1;
}

template<class ftype1,class ftype2>
matrix_tpl<ftype1>& operator-=(matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	for(int i=op1.nRows*op1.nCols-1; i>=0; i--) op1.data[i] -= op2.data[i];
	op1.flags = op1.flags & mtx_foreign_data | op1.flags & op2.flags & mtx_symmetric;
	return op1;
}

template<class ftype1,class ftype2>
matrix_tpl<ftype1> operator+(const matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	matrix_tpl<ftype1> res; res=op1; res+=op2;
	return res;
}

template<class ftype1,class ftype2>
matrix_tpl<ftype1> operator-(const matrix_tpl<ftype1> &op1, const matrix_tpl<ftype2> &op2) {
	matrix_tpl<ftype1> res; res=op1; res-=op2;
	return res;
}

template<class ftype1,class ftype2,class ftype3>
ftype3 *mul_vector_by_matrix(const matrix_tpl<ftype1> &mtx, const ftype2 *psrc,ftype3 *pdst) {
	int i,j;
	for(i=0;i<mtx.nRows;i++) for(pdst[i]=0,j=0;j<mtx.nCols;j++)
		pdst[i] += mtx.data[i*mtx.nCols+j]*psrc[j];
	return pdst;
}

typedef matrix_tpl<real> matrix;
typedef matrix_tpl<float> matrixf;

#if defined(LINUX)
	#define DECLARE_MTXNxM_POOL(ftype,sz) template<> ftype matrix_tpl<ftype>::mtx_pool[sz] = {}; template<> int matrix_tpl<ftype>::mtx_pool_pos=0; \
		template<> int matrix_tpl<ftype>::mtx_pool_size=sz;
#else
	#define DECLARE_MTXNxM_POOL(ftype,sz) template<> ftype matrix_tpl<ftype>::mtx_pool[sz]; template<> int matrix_tpl<ftype>::mtx_pool_pos=0; \
		template<> int matrix_tpl<ftype>::mtx_pool_size=sz;
#endif //LINUX

extern int g_bHasSSE;
#ifdef PIII_SSE
void PIII_Mult00_6x6_6x6(float *src1, float *src2, float *dst);
inline void matrix_product_tpl<float>::assign_to(float *pdst) const {
	if ((g_bHasSSE^1|nRows1-6|nCols1-6|nCols2-6)==0) 
		PIII_Mult00_6x6_6x6(data1,data2, pdst);
	else {
		int i,j,k; float sum;
		for(i=0;i<nRows1;i++) for(j=0;j<nCols2;j++) {
			for(sum=0,k=0; k<nCols1; k++) 
				sum += data1[i*nCols1+k]*data2[k*nCols2+j];
			pdst[i*nCols2+j] = sum;
		}
	}
}
#endif

#endif