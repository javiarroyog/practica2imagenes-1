/**
 * @file image.cpp
 * @brief Fichero con definiciones para los métodos primitivos de la clase Image
 *
 */

#include <cstring>
#include <cassert>
#include <iostream>
#include <math.h>

#include <image.h>
#include <imageIO.h>

using namespace std;

/********************************
      FUNCIONES PRIVADAS
********************************/
void Image::Allocate(int nrows, int ncols, byte * buffer){
    rows = nrows;
    cols = ncols;

    img = new byte * [rows];

    if (buffer != 0)
        img[0] = buffer;
    else
        img[0] = new byte [rows * cols];

    for (int i=1; i < rows; i++)
        img[i] = img[i-1] + cols;
}

// Función auxiliar para inicializar imágenes con valores por defecto o a partir de un buffer de datos
void Image::Initialize (int nrows, int ncols, byte * buffer){
    if ((nrows == 0) || (ncols == 0)){
        rows = cols = 0;
        img = 0;
    }
    else Allocate(nrows, ncols, buffer);
}

// Función auxiliar para copiar objetos Imagen

void Image::Copy(const Image & orig){
    Initialize(orig.rows,orig.cols);
    for (int k=0; k<rows*cols;k++)
        set_pixel(k,orig.get_pixel(k));
}

// Función auxiliar para destruir objetos Imagen
bool Image::Empty() const{
    return (rows == 0) || (cols == 0);
}

void Image::Destroy(){
    if (!Empty()){
        delete [] img[0];
        delete [] img;
    }
}

LoadResult Image::LoadFromPGM(const char * file_path){
    if (ReadImageKind(file_path) != IMG_PGM)
        return LoadResult::NOT_PGM;

    byte * buffer = ReadPGMImage(file_path, rows, cols);
    if (!buffer)
        return LoadResult::READING_ERROR;

    Initialize(rows, cols, buffer);
    return LoadResult::SUCCESS;
}

/********************************
       FUNCIONES PÚBLICAS
********************************/

// Constructor por defecto

Image::Image(){
    Initialize();
}

// Constructores con parámetros
Image::Image (int nrows, int ncols, byte value){
    Initialize(nrows, ncols);
    for (int k=0; k<rows*cols; k++) set_pixel(k,value);
}

bool Image::Load (const char * file_path) {
    Destroy();
    return LoadFromPGM(file_path) == LoadResult::SUCCESS;
}

// Constructor de copias

Image::Image (const Image & orig){
    assert (this != &orig);
    Copy(orig);
}

// Destructor

Image::~Image(){
    Destroy();
}

// Operador de Asignación

Image & Image::operator= (const Image & orig){
    if (this != &orig){
        Destroy();
        Copy(orig);
    }
    return *this;
}

// Métodos de acceso a los campos de la clase

int Image::get_rows() const {
    return rows;
}

int Image::get_cols() const {
    return cols;
}

int Image::size() const{
    return get_rows()*get_cols();
}

// Métodos básicos de edición de imágenes
void Image::set_pixel (int i, int j, byte value) {
    img[i][j] = value;
}
byte Image::get_pixel (int i, int j) const {
    return img[i][j];
}

// This doesn't work if representation changes
void Image::set_pixel (int k, byte value) {
    // TODO this makes assumptions about the internal representation
    // TODO Can you reuse set_pixel(i,j,value)?
    img[0][k] = value;
}

// This doesn't work if representation changes
byte Image::get_pixel (int k) const {
    // TODO this makes assumptions about the internal representation
    // TODO Can you reuse get_pixel(i,j)?
    return img[0][k];
}

// Métodos para almacenar y cargar imagenes en disco
bool Image::Save (const char * file_path) const {
    // TODO this makes assumptions about the internal representation
    byte * p = img[0];
    return WritePGMImage(file_path, p, rows, cols);
}

// Método para obtener una subimagen
Image Image :: Crop (int nrows, int ncol, int height, int width) const{
    Image subimagen(height,width);

    int i = 0, j = 0;

    for (int r = nrows; r < height + nrows; r++){
        for (int c = ncol; c < width + ncol; c++){
            subimagen.set_pixel(i,j,this->get_pixel(r,c));
            j++;
        }
        i++;
        j = 0;
    }
    return subimagen;
}

// Calcula la media de los píxeles de una imagen entera o de un fragmento de ésta.
double Image :: Mean(int i, int j, int height, int width) const{
    double suma = 0;
    double num_pixeles = height * width;

    for (int f = i; f < height + i; f++){
        for (int c = j; c < width + j; c++){
            suma += (double)get_pixel(f,c);
        }
    }
    suma = suma / num_pixeles;

    return suma + 0.5;
}

// Genera una imagen aumentada 2x.
Image Image :: Zoom2X() const{

    Image tmp((rows*2)-1, (cols*2)-1);

    // Interpolacion de columnas

    byte interpola_columnas;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols - 1; j++) {
            interpola_columnas = (byte)Mean(i, j, 1, 2);
            tmp.set_pixel(2 * i, 2 * j, get_pixel(i, j));
            tmp.set_pixel(2 * i, 2 * j + 1, interpola_columnas);
        }
        tmp.set_pixel(2 * i, tmp.cols - 1, get_pixel(i, cols - 1));
    }

    // Interpolacion de filas

    byte interpola_filas ;
    int c = 0;
    int f = 0;

    for (int i = 0; i < tmp.rows - 2; i+=2){
        for (int j = 0; j < tmp.cols; j++) {
            interpola_filas = 0;
            if ((j % 2) != 0) {
                interpola_filas = (byte)Mean(f, c, 2, 2);
                c++;
            }
            else
                interpola_filas = (byte) Mean(f, c, 2, 1);

            tmp.set_pixel(i+1,j, interpola_filas);

        }
        c = 0;
        f++;
    }



    return tmp;

}

// Genera un icono como reducción de una imagen.
Image Image :: Subsample(int factor) const{
    Image sub((rows / factor), (cols / factor));

    int f = 0;
    int c;
    byte tmp;

    for (int i = 0; i < sub.rows; i++){
        c = 0;
        for (int j = 0; j < sub.cols; j++){
            tmp = (byte)Mean(f,c,factor,factor);
            sub.set_pixel(i,j,tmp);
            c+=factor;
        }
        f+=factor;
    }
    return sub;

}

double Image :: TransfLineal (byte z, byte a,byte b, byte min,byte max){

    double t = min +  (((double)(max - min) / (b - a)) * (z - a));

    return round(t);

}
// Modifica el contraste de una Imagen
void Image :: AdjustContrast (byte in1, byte in2, byte out1, byte out2){

    byte min, max;
    byte a,b;

    for (int i = 0; i < rows*cols; i++){
        byte z = get_pixel(i);

        if ( z >= 0 && z < in1){
            min = 0;
            max = out1;
            a = 0;
            b = in1;
        }
        else if (z >= in1 && z <= in2){
            min = out1;
            max = out2;
            a = in1;
            b = in2;
        }
        else if (z > in2 && z <= 255){
            min = out2;
            max = 255;
            a = in2;
            b = 255;
        }

        double nuevo = (double)TransfLineal(z,a,b,min,max);

        set_pixel(i,nuevo);
    }
}

void Image::ShuffleRows() {
    const int p = 9973;
    Image temp(rows,cols);
    int newr;

    for (int r=0; r<rows; r++){
        newr = r*p%rows;
        for (int c=0; c<cols;c++)
            temp.set_pixel(r,c,get_pixel(newr,c));
    }
    Copy(temp);
}

