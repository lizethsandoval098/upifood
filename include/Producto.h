#ifndef PRODUCTO_H 
#define PRODUCTO_H

#include <string>
#include <iostream>

class BaseDatos;

using namespace std;

class Producto{
	private:
		string nombreProducto;
		string idProducto;
		int stock;
		float precio;

		BaseDatos& bd;

	public:
		Producto();
		Producto(string nombreProducto, string idProducto, int stock, float precio);

		~Producto();

		string getNombreProducto() const;
		string getIdProducto() const;
		int getStock() const;
		float getPrecio() const;

		void setNombreProducto(const string& nombrep);
		void setIdProducto(const string& idp);
		void setStock(int sto);
		void setPrecio(float pre);
};

#endif
		
