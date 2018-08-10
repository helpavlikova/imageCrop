/* 
 * File:   main.cpp
 * Author: Hell
 *
 * Created on 24. března 2016, 14:54
 */

#ifndef __PROGTEST__
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>
#include <climits>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <queue>
#include <stdint.h>
using namespace std;

const int ENDIAN_LITTLE = 0x49494949;
const int ENDIAN_BIG = 0x4d4d4d4d;

struct CRect {

    CRect(int x, int y, int w, int h)
    : m_X(x), m_Y(y), m_W(w), m_H(h) {
    }
    int m_X;
    int m_Y;
    int m_W;
    int m_H;
};
#endif /* __PROGTEST__ */

int readEndian(ifstream& infile) {
    int32_t endianity;
    infile.read((char*) &endianity, sizeof (int));
    //cout << hex << "endian: " << endianity << dec << endl;
    if (endianity != ENDIAN_LITTLE && endianity != ENDIAN_BIG) {
        //cout << "spatny endian" << endl;
        return 0;
    }
    return endianity;
}

int readInt(ifstream& infile) {
    int32_t tmp;
    infile.read((char*) &tmp, sizeof (int));
    return tmp;
}

int endianSwap(int x)
{
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    
    return x;
}

int makeCeil(int a, int b) {
    if (a % b) {
        return (a / b) + 1;
    } else {
        return a / b;
    }
}

int countPixelSize(int format) {
    int channel = format >> 3;
    //cout << "kanal: " << channel << endl;
    if (channel <= 0 || channel > 7) {
        return -1;
    }

    int bitPerChannel = format & 7;
    switch (bitPerChannel) {
        case 0:
            bitPerChannel = 1;
            break;
        case 2:
            bitPerChannel = 4;
            break;
        case 3:
            bitPerChannel = 8;
            break;
        case 4:
            bitPerChannel = 16;
            break;
        default:
            bitPerChannel = -1;
            break;
    }
    //cout << "bitu na slozku: " << bitPerChannel << endl;
    return channel* bitPerChannel;
}

bool writeHeader(ofstream& outfile, int endianity, int width, int height, int format) {
    outfile.write((char *) &endianity, sizeof (endianity));
    if (outfile.fail()) return false;
    outfile.write((char *) &width, sizeof (width));
    if (outfile.fail()) return false;
    outfile.write((char *) &height, sizeof (height));
    if (outfile.fail()) return false;
    outfile.write((char *) &format, sizeof (format));
    if (outfile.fail()) return false;

 	if(!outfile.good()) {return false;}
    return true;
}

bool writeWholeBytes(ifstream& infile, ofstream& outfile, const int pixelSize, const CRect & rc, int width, int byteOrder) {
    int len = rc.m_W * pixelSize;
    char * tmp = new char[len];
    int y = rc.m_Y;


    for (int i = 0; i < rc.m_H; i++) {
        infile.seekg((rc.m_X * pixelSize) + (width * y * pixelSize) + 16); //na offsetu 16 zacinaji data obrazku
        //cout << "Hledam pozici: " << (rc.m_X * pixelSize) + (width * y * pixelSize) + 16 << endl;

        for (int j = 0; j < len; j++)
            infile.read(&tmp[j], sizeof (char));
        /*for (int j = 0; j< len; j++)
            //cout << hex << (int)tmp[j] << dec << " ";
        //cout << endl;*/
        if(pixelSize == 2 && byteOrder == ENDIAN_BIG){
            for(int j = 0; j < len - 2 ; j+=2){
                char store = tmp[j];
                tmp[j] = tmp[j+1];
                tmp[j+1] = store;
            }
        }
        
        for (int j = 0; j < len; j++)
            outfile << tmp[j];
        if (outfile.fail()) return false;

 		if(!outfile.good()) {return false;}
        y++;
    }
    delete []tmp;
    return true;
}

int getMask(int count) {
    int res = 1;
    for (int i = 0; i < count; i++) {
        res *= 2;
    }
    return res - 1;
}

char mergeBytes(char helper, char byte2, int count) {
    helper = helper << count;
    byte2 = byte2 >> (8 - count);
    byte2 = byte2 & getMask(count);
    //cout << "helper: " <<hex << (int) helper <<dec << endl;
    byte2 = helper | byte2;
    //cout << "Zapisuji si: " << hex << (int) byte2 << dec << " ";
    return byte2;
}

bool writeHalfBytes(ifstream& infile, ofstream& outfile, const int pixelSize, const CRect & rc, int width) {
    int len = rc.m_W / 2 * pixelSize;
    char * tmp = new char[len];
    int y = rc.m_Y;
    int lineLenght;
    int x = rc.m_X / 2 * pixelSize;
    if (width % 2) {
        lineLenght = (width * pixelSize + 1) / 2;
    } else {
        lineLenght = (width * pixelSize) / 2;
    }

    if (x % 2 || (x == 0 && rc.m_X != 0)) {
        x++;
    }


    for (int i = 0; i < rc.m_H; i++) {
        infile.seekg((x) + (lineLenght * y) + 16);
        //cout << "Hledam pozici: " << (x) + (lineLenght * y) + 16 << endl;

        if (rc.m_X % 2) { //pokud pixel zacina v druhe polovine bajtu
            char helper, byte2;
            infile.read(&helper, sizeof (char));
            //cout << "Zacinam cist: " << hex << (int) helper << dec << endl;
            for (int j = 0; j < len; j++) {
                infile.read(&byte2, sizeof (char));
                tmp[j] = mergeBytes(helper, byte2, 4);
                helper = byte2;
            }
        } else {
            for (int j = 0; j < len; j++)
                infile.read(&tmp[j], sizeof (char));
        }

        for (int j = 0; j < len; j++){
            outfile << tmp[j];
 			if(!outfile.good()) {return false;}
        }


        if (rc.m_W % 2 && !(rc.m_X % 2)) { //pri liche velikosti vyrezu potrebujeme prvni 4 bity z dalsiho bytu
            char fourBites;
            infile.read(&fourBites, sizeof (char));
            fourBites = fourBites & 0xf0;
            outfile << fourBites;
            if(!outfile.good()) {return false;}
        }
        if (outfile.fail()) return false;
        y++;
    }
    delete []tmp;

    return true;
}

bool writeBits(ifstream& infile, ofstream& outfile, const CRect & rc, int width, const int pixelSize) {
    int len = makeCeil(rc.m_W * pixelSize, 8);
    char * tmp = new char[len];
    int y = rc.m_Y;
    int offset;
    int lineLenght = makeCeil(width * pixelSize, 8);
    int padding = (rc.m_W * pixelSize) % 8;
    if((rc.m_X * pixelSize) % 8){
        offset = 15;
    } 
    else{
    offset = 16;
    }
    

    for (int i = 0; i < rc.m_H; i++) {
        infile.seekg(makeCeil(rc.m_X * pixelSize, 8) + (lineLenght * y) + offset); //proc 15 a ne 16? chjo.
        //cout << "Hledam pozici: " << makeCeil(rc.m_X * pixelSize, 8) << " + " <<(lineLenght * y) <<" + " << offset << endl;
        

        char helper, byte2;
        infile.read(&helper, sizeof (char));
        //cout << "Zacinam cist: " << hex << (int) helper << dec << endl;
        for (int j = 0; j < len; j++) {
            infile.read(&byte2, sizeof (char));
            //cout << "Ctu dalsi bajt: " << hex << (int) byte2 << dec << endl;
            if(j == len - 1 && j != 0){
                byte2 = mergeBytes(helper, byte2, rc.m_X * pixelSize);
                byte2 = byte2 >> (8 - padding);
                byte2 = byte2 << (8 - padding);
                tmp[j] = byte2;
            }else{
                tmp[j] = mergeBytes(helper, byte2, rc.m_X * pixelSize);
            }
            helper = byte2;
            //cout << endl;
        }
        for (int j = 0; j < len; j++){
            outfile << tmp[j];
 			if(!outfile.good()) {return false;}
        }
        if (outfile.fail()) return false;
        
        y++;
    }

    delete [] tmp;
    return true;
}

bool cropImage(const char * srcFileName, const char * dstFileName, const CRect & rc, int byteOrder) {
	if(rc.m_X < 0 || rc.m_Y < 0 || rc.m_W <= 0 || rc.m_H <= 0){
		return false;
	}


    ifstream infile;
    infile.open(srcFileName, ios::binary | ios::ate);
    if (infile.fail() || !infile.is_open()) {
        //cout << "Nelze otevrit soubor" << endl;
        
        return false;
    }
    streampos size = infile.tellg();
    infile.seekg(0, ios::beg);
    if(size <= 0){
    	return false;
    }

    int endianity = readEndian(infile);
    if (!endianity) {
        //cout << "readEndian vraci false" << endl;
        infile.close();
        return false;
    }

    
    int32_t width = readInt(infile);
    int32_t height = readInt(infile);
    int32_t format = readInt(infile);

    if(endianity == ENDIAN_BIG){
        width = endianSwap(width);
        height = endianSwap(height);
        format = endianSwap(format);
    }
    
    
    if(width <= 0 || height <= 0){
    	infile.close();
    	return false;
    }

   // cout << "sirka: " << width << endl << "vyska: " << height << endl; //<< "format: " << format << endl;
    
    if(rc.m_X + rc.m_W > width || rc.m_Y + rc.m_H > height){
        infile.close();
        return false;
    }

    int pixelSize = countPixelSize(format);
    if (pixelSize < 0) {
        //cout << "countPixelSize vraci false" << endl;
        
        infile.close();
        return false;
    }

    if( (width * height * pixelSize) / 8 + 16 > size){
    	//cout << "Spatne rozmery  obrazku." <<endl;

    	infile.close();
    	return false;
    }
    
   // cout << "velikost 1 pixelu: " << pixelSize << endl;

    ofstream outfile;
    outfile.open(dstFileName, ios::binary | ios::out);
    if(!outfile.is_open() || outfile.fail() ) {
        infile.close();
    	outfile.flush();
    	outfile.close();
        return false;
    }
    
    int m_W, m_H;
    if(endianity == ENDIAN_LITTLE){
        m_H = rc.m_H;
        m_W = rc.m_W;
    }else{
        m_H = endianSwap(rc.m_H);
        m_W = endianSwap(rc.m_W);
        format = endianSwap(format);
    }

    if (!writeHeader(outfile, endianity, m_W, m_H, format)) {

    	infile.close();
    	outfile.flush();
    	outfile.close();
    	return false;
    }
    
   /* if(rc.m_X == 0 && rc.m_Y == 0 && rc.m_W == width && rc.m_H == height){
        copyFile(infile, outfile, size);
    }*/
    if ((pixelSize % 8) == 0)
        writeWholeBytes(infile, outfile, pixelSize / 8, rc, width, byteOrder);
    else if ((pixelSize % 4) == 0)
        writeHalfBytes(infile, outfile, pixelSize / 4, rc, width);
    else {
        writeBits(infile, outfile, rc, width, pixelSize);
    }

    outfile.flush();
    infile.close();
    outfile.close();
    return true;
}

#ifndef __PROGTEST__

bool identicalFiles(const char * fileName1, const char * fileName2) {
    ifstream infile1(fileName1, ios::binary | ios::ate);
    ifstream infile2(fileName2, ios::binary | ios::ate);
    if (infile1.fail() || infile2.fail()) {
        //cout << "Nelze otevrit soubor" << endl;
        return false;
    }
    
    streampos size1 = infile1.tellg();
    infile1.seekg(0, ios::beg);
    streampos size2 = infile2.tellg();
    infile2.seekg(0, ios::beg);

    if (size1 != size2) {
        return false;
    }

    char * memblock1;
    char * memblock2;
    memblock1 = new char [size1];
    infile1.read(memblock1, size1);
    infile1.close();


    memblock2 = new char [size1];
    infile2.read(memblock2, size1);
    infile2.close();

    for (int i = 0; i < size1; i++) {
        if (memblock1[i] != memblock2[i]) {
            delete[]memblock2;
            delete[]memblock1;
            return false;
        }
    }

    delete[]memblock2;
    delete[]memblock1;
    return true;

}

int main(void) {
    
     assert (  cropImage ( "input_00.raw", "output_00.raw", CRect ( 1, 2, 2, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_00.raw", "ref_00.raw" ) );

  assert (  cropImage ( "input_01.raw", "output_01.raw", CRect ( 1, 2, 2, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_01.raw", "ref_01.raw" ) );

  assert (  cropImage ( "input_02.raw", "output_02.raw", CRect ( 1, 2, 2, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_02.raw", "ref_02.raw" ) );

  assert (  cropImage ( "input_03.raw", "output_03.raw", CRect ( 0, 1, 3, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_03.raw", "ref_03.raw" ) );

  assert (  cropImage ( "input_04.raw", "output_04.raw", CRect ( 0, 1, 3, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_04.raw", "ref_04.raw" ) );

  assert (  cropImage ( "input_05.raw", "output_05.raw", CRect ( 1, 1, 4, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_05.raw", "ref_05.raw" ) );

  assert (  cropImage ( "input_06.raw", "output_06.raw", CRect ( 2, 2, 14, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_06.raw", "ref_06.raw" ) );

  assert (  cropImage ( "input_07.raw", "output_07.raw", CRect ( 2, 2, 8, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_07.raw", "ref_07.raw" ) );

  assert (  cropImage ( "input_08.raw", "output_08.raw", CRect ( 2, 2, 7, 3 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_08.raw", "ref_08.raw" ) );

  assert (  cropImage ( "input_09.raw", "output_09.raw", CRect ( 2, 2, 4, 4 ), ENDIAN_LITTLE )
         && identicalFiles ( "output_09.raw", "ref_09.raw" ) );

  assert ( ! cropImage ( "input_10.raw", "output_10.raw", CRect ( 2, 2, 10, 3 ), ENDIAN_LITTLE ) );

  assert ( ! cropImage ( "input_11.raw", "output_11.raw", CRect ( 2, 11, 6, 3 ), ENDIAN_LITTLE ) );
  

  
  bool crop = cropImage ( "in_1797018.bin", "tests.raw", CRect(25,4,14,31), ENDIAN_BIG );
  bool identity =  identicalFiles ( "tests.raw", "ref_1797018.bin" ) ;
  
  cout << "cropImage vraci: " << crop << endl;
  cout << "identicalFiles vraci:" << identity << endl;

    return 0;
}
#endif /* __PROGTEST__ */


