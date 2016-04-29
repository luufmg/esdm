// This file is part of h5-memvol.
//
// SCIL is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SCIL is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with h5-memvol.  If not, see <http://www.gnu.org/licenses/>.

// TODO some locking here
static GHashTable * type_table = NULL;

static void memvol_init_dtype(hid_t vipl_id){
  type_table =  g_hash_table_new (g_str_hash, g_str_equal);
}

static void * memvol_datatype_commit(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req)
{
  g_hash_table_insert(type_table, (char*) name, (void*) type_id);
  printf("C %p\n", (void*) type_id);

  return (void*) type_id;
}

static void * memvol_datatype_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t tapl_id, hid_t dxpl_id, void **req)
{
  void * found = g_hash_table_lookup(type_table, name);
  printf("O %p\n", found);
  hid_t tid = H5Tcreate (H5T_COMPOUND, 10);

  return (void*)tid;
}


static herr_t memvol_datatype_get(void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
  switch (get_type) {
      case H5VL_DATATYPE_GET_BINARY:{ // serialize datatype
        ssize_t *nalloc = va_arg (arguments, ssize_t *);
        void *buf = va_arg (arguments, void *);
        size_t size = va_arg (arguments, size_t);
        break;
      }
      case H5VL_DATATYPE_GET_TCPL:{ // property list when the datatype has been created
        hid_t *ret_id = va_arg (arguments, hid_t *);
        *ret_id = H5P_DEFAULT;
        break;
      }
  }
  return 0;
}


static herr_t memvol_datatype_close(void *dt, hid_t dxpl_id, void **req)
{
  return 0;
}