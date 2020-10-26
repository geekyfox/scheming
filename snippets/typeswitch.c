// type switch
void reach(object_t obj)
{
	switch (obj->type)
	{
	case TYPE_STRING:
		reach_string(obj);
		break;
	case TYPE_PAIR:
		reach_pair(obj);
		break;
	...
	}
}