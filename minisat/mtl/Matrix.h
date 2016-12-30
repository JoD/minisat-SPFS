#pragma once

#include "minisat/mtl/IntTypes.h"
#include "minisat/mtl/Vec.h"

namespace Minisat {

template <typename T>
struct matrix {
    int nr; // number of rows
    int nc; // number of columns
    vec<T> m; // such that matrix[r[l]*columns+c[l]] == l

    matrix():nr(0),nc(0){
        m.growTo(0);
    }

    matrix(int nrows, int ncolumns){
        nr=nrows;
        nc=ncolumns;
        m.growTo(nr*nc);
    }
    
    void growTo(int nbRows, int nbColumns){
        if(nbRows*nbColumns<=m.size()){
            return;
        }
        nr=nbRows;
        nc=nbColumns;
        m.growTo(nr*nc);
    }

    void growTo(int nbRows, int nbColumns, const T& e){
        if(nbRows*nbColumns<=m.size()){
            return;
        }
        nr=nbRows;
        nc=nbColumns;
        m.growTo(nr*nc,e);
    }

    void addRow(){
        ++nr;
        m.growTo(nr*nc);
    }

    void assign(int nbRows, int nbColumns, const T& e){
        nr=nbRows;
        nc=nbColumns;
        m.assign(nr*nc,e);
    }

    void set(int r, int c, const T& e){
        assert(r<nr);
        assert(r>=0);
        assert(c<nc);
        assert(c>=0);
        m[r*nc+c]=e;
    }

    T& get(int r, int c){
        assert(r<nr);
        assert(r>=0);
        assert(c<nc);
        assert(c>=0);
        return m[r*nc+c];
    }

    void copyTo(matrix<T>& other){
        other.nr=nr;
        other.nc=nc;
        other.m=m;
    }

    inline int size(){
        return m.size();
    }
};

//=================================================================================================
}
